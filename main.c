#include <math.h>
#include "calculations.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MY_BACKGROUND_COLOR 0x111418ff // dark grey
#define MY_TEXT_COLOR 0Xccccccff // white
#define MY_BUTTON_COLOR_NORMAL 0xA6A6A6FF // light grey
#define MY_BUTTON_COLOR_FOCUSED 0xCCCCCCFF // lighter grey
#define MY_BUTTON_COLOR_PRESSED 0x808080FF // grey

#define START_A (PI/4)
#define START_B (-PI/2)


//----------------------------------------------------------------------------------
// TYPES AND STRUCTURES DEFINITION
//----------------------------------------------------------------------------------


typedef struct {

    int computingFPS; // computation FPS (time steps per second)
    int drawingFPS; //  rendering FPS

    double time; // time of the simulation
    double frameTimeAccumulator; // accumulator for the time between drawn frames
    double timeSpeed; // speed of time as a multiplier of real time (for example 2 would mean time goes twice as fast)

    Color background_color;
    Color text_color;

    bool running; // true while the simulation is running
    bool btnStopContinue; // true when Stop/Continue button is pressed
    bool btnRestart; // true when Restart button is pressed

    // when true makes computingFPS use simulated (sped-up) time maintaining CPU cost constant
    // if false, CPU cost will grow with timeSpeed (speed time would only affect drawn frames)
    bool computeOnSimulatedTime; 

    Vector stateVector; // Vector with the state of the angles and angular velocities
    Constraints system; // Constraints of the system

} State;


//----------------------------------------------------------------------------------
// MODULE FUNCTIONS DECLARATION
//----------------------------------------------------------------------------------

// Frame update function (has to consume a void pointer to the state to be compatible with emscripten_set_main_loop_arg)
void UpdateFrame(void *void_state);

// Calculates a step on the simulation
void StepSimulation(State *state, double dt);

// Draws the simulation
void DrawSimulation(State *state);

// Draws the screen when the simulation is paused and before it starts
void DrawPauseScreen(State *state);

//------------------------------------------------------------------------------------
// PROGRAM MAIN ENTRY POINT
//------------------------------------------------------------------------------------

int main(void){

    //--------------------------------------------------------------------------------------
    // SETUP
    //--------------------------------------------------------------------------------------

    const int screenWidth = 800;
    const int screenHeight = 450;

    State state = {

        .computingFPS = 120,
        .drawingFPS = 60,

        .time = 0.0,
        .frameTimeAccumulator = 0.0,
        .timeSpeed = 1.0,
        
        .background_color = {0x11, 0x14, 0x18, 0xff},
        .text_color = {0x91, 0x91, 0x91, 0xff},

        .running = false,
        .btnStopContinue = false,
        .btnRestart = false,

        .stateVector = {
            .a = START_A,
            .a_ = 0.0,
            .b = START_B,
            .b_ = 0.0
        },

        .system = {

            .l1 = 200.0,
            .l2 = 150.0,
            .g = 981,
            .m1 = 100,
            .m2 = 100,
            .c1 = 5e-4,
            .c2 = 5e-4
        }

    };


    InitWindow(screenWidth, screenHeight, "Simulation");
    SetTargetFPS(state.drawingFPS);
    
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetStyle( DEFAULT, TEXT_COLOR_NORMAL, MY_TEXT_COLOR); 
    GuiSetStyle( DEFAULT, BACKGROUND_COLOR, MY_BACKGROUND_COLOR); 

    GuiSetStyle( BUTTON, TEXT_COLOR_NORMAL, MY_BACKGROUND_COLOR); 
    GuiSetStyle( BUTTON, TEXT_COLOR_FOCUSED, MY_BACKGROUND_COLOR); 
    GuiSetStyle( BUTTON, TEXT_COLOR_PRESSED, MY_BACKGROUND_COLOR); 

    GuiSetStyle( BUTTON, BASE_COLOR_NORMAL, MY_BUTTON_COLOR_NORMAL); 
    GuiSetStyle( BUTTON, BASE_COLOR_FOCUSED, MY_BUTTON_COLOR_FOCUSED); 
    GuiSetStyle( BUTTON, BASE_COLOR_PRESSED, MY_BUTTON_COLOR_PRESSED); 

    GuiSetStyle( BUTTON, BORDER_COLOR_NORMAL, MY_BUTTON_COLOR_NORMAL); 
    GuiSetStyle( BUTTON, BORDER_COLOR_FOCUSED, MY_BUTTON_COLOR_FOCUSED); 
    GuiSetStyle( BUTTON, BORDER_COLOR_PRESSED, MY_BUTTON_COLOR_PRESSED); 


    GuiSetIconScale(2);



    //--------------------------------------------------------------------------------------
    // MAIN LOOP
    //--------------------------------------------------------------------------------------

    #if defined(PLATFORM_WEB)
        emscripten_set_main_loop_arg(UpdateFrame, &state, state.drawingFPS, 1);
    #else
        while (!WindowShouldClose()) UpdateFrame(&state);
    #endif

    //--------------------------------------------------------------------------------------
    // DE-INITIALIZATION
    //--------------------------------------------------------------------------------------

    CloseWindow();

    return 0;
}

void UpdateFrame(void *void_state) {

    // -------------------------------------------
    // SETUP
    // -------------------------------------------

    State *state = (State *)void_state;
    const double dt = (state->computeOnSimulatedTime) ? state->timeSpeed/state->computingFPS : 1.0/state->computingFPS;

    // -------------------------------------------
    // LISTEN FOR PRESSES AND UPDATE VARIABLES
    // -------------------------------------------

    if (state->btnStopContinue || IsKeyPressed(KEY_SPACE)) {
        state->running = !state->running;
        state->btnStopContinue = false;
    }


    if (state->btnRestart || IsKeyPressed(KEY_R)) {

        // Restart simulation parameters
        state->time = 0.0f;
        state->frameTimeAccumulator = 0.0f;

        state->stateVector.a = START_A;
        state->stateVector.a_ = 0.0f;
        state->stateVector.b = START_B;
        state->stateVector.b_ = 0.0f;
        

        // IMPORTANT TO RESTART SIMULATION-SPECIFIC PARAMETERS TOO
        state->btnRestart = false;
    }


    // -------------------------------------------
    // DRAWING BEGINS
    // -------------------------------------------

    if (state->running) {

        state->frameTimeAccumulator += GetFrameTime() * state->timeSpeed;

        // Computes more than one timestep of the simulation each frame so that the simulation runs at computingFPS
        while (state->frameTimeAccumulator >= dt) { 

            StepSimulation(state, dt);
            state->time += dt;
            state->frameTimeAccumulator -= dt;

        }

        DrawSimulation(state);

    } else DrawPauseScreen(state);

}

void StepSimulation(State *state, double dt){

    rungekutta(pendulumEquations, state->time, &state->stateVector, dt, &state->system);

}

void DrawSimulation(State *state) {

    double x_joint = GetScreenWidth()/2 + state->system.l1*cos(state->stateVector.a);
    double y_joint = state->system.l1*sin(state->stateVector.a);

    double x_mass = GetScreenWidth()/2 + state->system.l1*cos(state->stateVector.a) + state->system.l2*cos(state->stateVector.b);
    double y_mass = state->system.l1*sin(state->stateVector.a) + state->system.l2*sin(state->stateVector.b);

    BeginDrawing();
    ClearBackground(state->background_color);

    // Simulation drawing

    // First segment
    DrawLine(GetScreenWidth()/2, 0, x_joint, y_joint, RAYWHITE);

    // Second segment
    DrawLine(x_joint, y_joint, x_mass, y_mass, RAYWHITE);


    // Mass 1
    DrawCircle( x_joint, y_joint, state->system.m1/10, DARKBLUE);

    // Mass 2
    DrawCircle( x_mass, y_mass, state->system.m2/10, DARKBLUE);

    EndDrawing();

}

void DrawPauseScreen(State *state) {

    BeginDrawing();

    ClearBackground(state->background_color);

    // Draw instructions
    //GuiLabel((Rectangle){ 200, 100, 2300, 36 }, "Use the BUTTONS or KEY SHORTCUTS:");
    DrawText("Use the BUTTONS or KEY SHORTCUTS:", 10, GetScreenHeight()*9/10, 20, state->text_color);


    // Draw buttons
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    state->btnStopContinue = GuiButton((Rectangle){ sw/3, sw/10, sw/3, sh/10}, (state->time != 0.0) ? "RESUME [SPACE]" : "START [SPACE]");
    state->btnRestart = GuiButton((Rectangle){ sw/3, sw*2/10, sw/3, sh/10}, "RESTART [R]");

    EndDrawing();
}