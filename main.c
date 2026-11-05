#include <float.h>
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

#define START_A 0//(PI/4)
#define START_B 0//(-PI/2)

#define INPUT_CAP_TEXT_BUFFER 20
#define NUM_FORMAT "%g"

//----------------------------------------------------------------------------------
// TYPES AND STRUCTURES DEFINITION
//----------------------------------------------------------------------------------


typedef struct {

    const int screenWidth;
    const int screenHeight;

    int computingFPS; // computation FPS (time steps per second)
    int drawingFPS; //  rendering FPS

    double time; // time of the simulation
    double frameTimeAccumulator; // accumulator for the time between drawn frames
    double timeSpeed; // speed of time as a multiplier of real time (for example 2 would mean time goes twice as fast)

    Color background_color;
    Color text_color;

    int running; // 0 for start menu, 1 for simulation running, 2 for configuration menu
    bool btnStopContinue; // true when Stop/Continue button is pressed
    bool btnRestart; // true when Restart button is pressed
    bool btnSettings; // true when Settings button is pressed
    bool btnGoBackToMenu; // true when Go Back To Menu button is pressed

    int focus; // int representing the text field that's focused at the moment or -1 if nothing is
    char buffer[INPUT_CAP_TEXT_BUFFER]; // buffer for text input in whichever text field is focused at the moment
    
    // when true makes computingFPS use simulated (sped-up) time maintaining CPU cost constant
    // if false, CPU cost will grow with timeSpeed (speed time would only affect drawn frames)
    bool computeOnSimulatedTime; 

    Vector initialState; // Vector with the initial state
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

// Draws the settings screen
void DrawSettings(State *state);

// Listen for text input on the text fields in Settings
void SettingsTextFields(State *state);

// Changes the value on the given adress from a character buffer
void ChangeValue(double *value, char buffer[20]);

// Draws an example diagram (for Settings)
void DrawDiagram(State *state);

//------------------------------------------------------------------------------------
// PROGRAM MAIN ENTRY POINT
//------------------------------------------------------------------------------------

int main(void){

    //--------------------------------------------------------------------------------------
    // SETUP
    //--------------------------------------------------------------------------------------

    
    State state = {

        .screenWidth = 800,
        .screenHeight = 450,

        .computingFPS = 120,
        .drawingFPS = 60,

        .time = 0.0,
        .frameTimeAccumulator = 0.0,
        .timeSpeed = 1.0,
        
        .background_color = {0x11, 0x14, 0x18, 0xff},
        .text_color = {0x91, 0x91, 0x91, 0xff},

        .running = 0,
        .btnStopContinue = false,
        .btnRestart = false,
        .btnSettings = false,
        .btnGoBackToMenu = false,

        .focus = -1,

        .initialState = {
            .a = START_A,
            .a_ = 0.0,
            .b = START_B,
            .b_ = 0.0
        },

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


    InitWindow(state.screenWidth, state.screenHeight, "Simulation");
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

    GuiSetStyle( TEXTBOX, TEXT_COLOR_NORMAL, MY_TEXT_COLOR); 
    GuiSetStyle( TEXTBOX, TEXT_COLOR_FOCUSED, MY_TEXT_COLOR); 
    GuiSetStyle( TEXTBOX, TEXT_COLOR_PRESSED, MY_TEXT_COLOR); 

    GuiSetStyle( TEXTBOX, BASE_COLOR_NORMAL, MY_BUTTON_COLOR_NORMAL); 
    GuiSetStyle( TEXTBOX, BASE_COLOR_FOCUSED, MY_BUTTON_COLOR_FOCUSED); 
    GuiSetStyle( TEXTBOX, BASE_COLOR_PRESSED, MY_BUTTON_COLOR_PRESSED); 

    GuiSetStyle( TEXTBOX, BORDER_COLOR_NORMAL, MY_BUTTON_COLOR_NORMAL); 
    GuiSetStyle( TEXTBOX, BORDER_COLOR_FOCUSED, MY_BUTTON_COLOR_FOCUSED); 
    GuiSetStyle( TEXTBOX, BORDER_COLOR_PRESSED, MY_BUTTON_COLOR_PRESSED); 

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

    if (state->running < 2) { // Handle common buttons in screen 0 and 1

        if (state->btnStopContinue || IsKeyPressed(KEY_SPACE)) {
            state->running = !state->running;
            state->btnStopContinue = false;
        }

        if (state->btnRestart || IsKeyPressed(KEY_R)) {

            // Restart simulation parameters
            state->time = 0.0f;
            state->frameTimeAccumulator = 0.0f;

            state->stateVector.a = state->initialState.a;
            state->stateVector.a_ = state->initialState.a_;
            state->stateVector.b = state->initialState.b;
            state->stateVector.b_ = state->initialState.b_;

            state->btnRestart = false;
        }

        if (state->btnSettings || IsKeyPressed(KEY_S)) {
            state->running = 2;
            state->btnSettings = false;
        }

    }

    else if (state->running == 2) {

        if (state->btnGoBackToMenu || IsKeyPressed(KEY_B)) {
            state->running = 0;
            state->btnGoBackToMenu = false;
        }

    }

    // -------------------------------------------
    // DRAWING BEGINS
    // -------------------------------------------

    switch (state->running) {

        case 0:

            DrawPauseScreen(state);
            break;

        case 1:

            state->frameTimeAccumulator += GetFrameTime() * state->timeSpeed;

            // Computes more than one timestep of the simulation each frame so that the simulation runs at computingFPS
            while (state->frameTimeAccumulator >= dt) { 

                StepSimulation(state, dt);
                state->time += dt;
                state->frameTimeAccumulator -= dt;

            }

            DrawSimulation(state);
            break;

        case 2:

            DrawSettings(state);
            break;
        
        default: break;

    }

}

void StepSimulation(State *state, double dt){

    rungekutta(pendulumEquations, state->time, &state->stateVector, dt, &state->system);

}

void DrawSimulation(State *state) {

    double x_joint = state->screenWidth/2 + state->system.l1*cos(state->stateVector.a);
    double y_joint = state->system.l1*sin(state->stateVector.a);

    double x_mass = state->screenWidth/2 + state->system.l1*cos(state->stateVector.a) + state->system.l2*cos(state->stateVector.b);
    double y_mass = state->system.l1*sin(state->stateVector.a) + state->system.l2*sin(state->stateVector.b);

    BeginDrawing();
    ClearBackground(state->background_color);

    // Simulation drawing

    // First segment
    DrawLine(state->screenWidth/2, 0, x_joint, y_joint, RAYWHITE);

    // Second segment
    DrawLine(x_joint, y_joint, x_mass, y_mass, RAYWHITE);


    // Mass 1
    DrawCircle( x_joint, y_joint, state->system.m1/10, DARKBLUE);

    // Mass 2
    DrawCircle( x_mass, y_mass, state->system.m2/10, DARKBLUE);

    EndDrawing();

}

void DrawPauseScreen(State *state) {

    const int sw = state->screenWidth;
    const int sh = state->screenHeight;

    BeginDrawing();

    ClearBackground(state->background_color);

    // Draw instructions
    //GuiLabel((Rectangle){ 200, 100, 2300, 36 }, "Use the BUTTONS or KEY SHORTCUTS:");
    DrawText("Use the BUTTONS or KEY SHORTCUTS:", sw/48, sh*90/96, 20, state->text_color);


    // Draw buttons

    state->btnStopContinue = GuiButton((Rectangle){ sw/3, sw/10, sw/3, sh/10}, (state->time != 0.0) ? "RESUME [SPACE]" : "START [SPACE]");
    state->btnRestart = GuiButton((Rectangle){ sw/3, sw*2/10, sw/3, sh/10}, "RESTART [R]");
    state->btnSettings = GuiButton((Rectangle){ sw/3, sw*3/10, sw/3, sh/10}, "SETTINGS [S]");

    EndDrawing();
}

void DrawSettings(State *state) {
    
    const int sw = state->screenWidth;
    const int sh = state->screenHeight;

    BeginDrawing();

    ClearBackground(state->background_color);

    // Draw text input fields
    SettingsTextFields(state);

    // Draw diagram of the variables
    DrawDiagram(state);
    
    // Draw instructions
    DrawText("Use the BUTTONS or KEY SHORTCUTS:", sw/48, sh*90/96, 20, state->text_color);

    // Draw buttons
    state->btnGoBackToMenu = GuiButton((Rectangle){ sw*41/48, sh*7/8, sw/8, sh/10}, "BACK [B]");

    EndDrawing();
}

void SettingsTextFields(State *state) {

    // If the mouse hovers the text field the cursor is changed and if the user clicks the focus changes to the field
    /* if (CheckCollisionPointRec(GetMousePosition(), box)) {
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) state->focus = textFieldIndex;
    } */

    if (state->focus >= 0 && IsKeyPressed(KEY_ESCAPE)) state->focus = -1;

    char tmp[INPUT_CAP_TEXT_BUFFER];
    bool focused;

    const int sw = state->screenWidth;
    const int sh = state->screenHeight;

    // Bounds for the text input fields in settings
    Rectangle bounds[12] = {

        { sw/3, sh*0.5/10, sw/8, sh/10}, 
        { sw/3, sh*1.5/10, sw/8, sh/10},
        { sw/3, sh*2.5/10, sw/8, sh/10},
        { sw/3, sh*3.5/10, sw/8, sh/10},
        { sw/3, sh*4.5/10, sw/8, sh/10},
        { sw/3, sh*5.5/10, sw/8, sh/10},
        { sw/3, sh*6.5/10, sw/8, sh/10},
        { sw/3, sh*7.5/10, sw/8, sh/10},
        { sw*5/6, sh*0.5/10, sw/8, sh/10}, 
        { sw*5/6, sh*1.5/10, sw/8, sh/10},
        { sw*5/6, sh*2.5/10, sw/8, sh/10},
        { sw*5/6, sh*3.5/10, sw/8, sh/10},

    };

    double *editable_values[12] = {

        &state->timeSpeed,
        &state->system.l1,
        &state->system.l2,
        &state->system.g,
        &state->system.m1,
        &state->system.m2,
        &state->system.c1,
        &state->system.c2,
        &state->initialState.a,
        &state->initialState.a_,
        &state->initialState.b,
        &state->initialState.b_

    };

    char names[12][40] = {

        "Time speed multiplier: ",
        "Size of first rod: ",
        "Size of second rod: ",
        "Gravity: ",
        "Mass 1 (joint): ",
        "Mass 2: ",
        "Drag coefficient 1: ",
        "Drag coefficient 2: ",
        "Initial angle 1: ",
        "Initial angular velocity 1: ",
        "Initial angle 2: ",
        "Initial angular velocity 2: "

    };

    for (int i = 0; i < 12; i++) {

        focused = state->focus == i;

        if (!focused) { // If not focused the field draws the current value
            snprintf(tmp, sizeof(tmp), NUM_FORMAT, *editable_values[i]);
        }

        if (GuiTextBox(bounds[i], (focused) ? state->buffer : tmp, INPUT_CAP_TEXT_BUFFER, focused)) {
            
            if (focused && IsKeyPressed(KEY_ENTER)) {
                ChangeValue(editable_values[i], state->buffer);
            }
            
            if (!focused) {
                state->focus = i;
                sprintf(state->buffer, "%s", "");
            } else state->focus = -1;

        };

        // First and second column need different padding between label and text box
        if (i < 8) DrawText(names[i], bounds[i].x-225, bounds[i].y+bounds[i].height/5, 20, state->text_color);
        else DrawText(names[i], bounds[i].x-270, bounds[i].y+bounds[i].height/5, 20, state->text_color);

    }
}

void ChangeValue(double *value, char buffer[20]) {

    // accepting , and .
    for (char *p = buffer; *p; p++) if (*p == ',') *p = '.';

    // pointer magic to convert str to double
    char *end;
    double num = strtod(buffer, &end);
    while (*end == ' ') end++;

    // value is updated num is worthy
    if (end != buffer && *end == '\0' && num >= -DBL_MAX && num <= DBL_MAX) {
        *value = num;
    }

}

void DrawDiagram(State *state) {
    
    // Example diagram of the variables (fixed shape, angle 1 = 40 deg, angle 2 = 35 deg)
    const float x0 = state->screenWidth/2, y0 = state->screenHeight*0.48f;   // top-left corner of the diagram area
    const Vector2 pivot = { x0 + 50,  y0 + 29 };
    const Vector2 joint = { x0 + 119, y0 + 87 };
    const Vector2 mass  = { x0 + 172, y0 + 124 };

    DrawLineDashed(pivot, (Vector2){ pivot.x + 50, pivot.y }, 4, 3, state->text_color);   // angle 1 reference
    DrawLineDashed(joint, (Vector2){ joint.x + 50, joint.y }, 4, 3, state->text_color);   // angle 2 reference
    DrawRing(pivot, 29, 31, 0, 40, 16, GOLD);
    DrawRing(joint, 29, 31, 0, 35, 16, GOLD);
    DrawLineEx(pivot, joint, 2, RAYWHITE);
    DrawLineEx(joint, mass, 2, RAYWHITE);
    DrawCircleV(pivot, 3, state->text_color);
    DrawCircleV(joint, 6, DARKBLUE);
    DrawCircleV(mass, 8, DARKBLUE);

    DrawText("angle 1", pivot.x + 56, pivot.y + 4, 10, GOLD);
    DrawText("angle 2", joint.x + 56, joint.y + 4, 10, GOLD);
    DrawText("rod 1", x0 + 58, y0 + 66, 10, state->text_color);
    DrawText("rod 2", x0 + 118, y0 + 110, 10, state->text_color);
    DrawText("mass 1", joint.x - 52, joint.y - 4, 10, state->text_color);
    DrawText("mass 2", mass.x + 14, mass.y - 4, 10, state->text_color);

    DrawLineEx((Vector2){ x0 + 330, y0 + 20 }, (Vector2){ x0 + 330, y0 + 46 }, 2, state->text_color);   // gravity arrow
    DrawTriangle((Vector2){ x0 + 330, y0 + 54 }, (Vector2){ x0 + 335, y0 + 46 }, (Vector2){ x0 + 325, y0 + 46 }, state->text_color);
    DrawText("g", x0 + 316, y0 + 28, 10, state->text_color);

    DrawText("Angles are measured clockwise from the dashed lines", x0 + 10, y0 + 145, 10, GOLD);
    DrawText("Positive angular velocity = clockwise rotation", x0 + 10, y0 + 157, 10, GOLD);
}