<div align="center">

# double-pendulum

**A double pendulum physics simulation**

![C](https://img.shields.io/badge/C-C99-00599C?logo=c&logoColor=white)
![raylib](https://img.shields.io/badge/raylib-6.2-black?logo=raylib&logoColor=white)
![raygui](https://img.shields.io/badge/raygui-4.0-black?logo=raylib&logoColor=white)
![Make](https://img.shields.io/badge/build-Make-A42E2B?logo=gnu&logoColor=white)
![Web](https://img.shields.io/badge/web-WebAssembly-654FF0?logo=webassembly&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Web-lightgrey)
![Status](https://img.shields.io/badge/status-in%20development-orange)

</div>

- The motion is computed from the equations derived from the Lagrangian of the system and integrated with a 4th order Runge-Kutta method.

- The simulation runs on a fixed time step (120 steps per second by default) that is independent from the rendering rate (60 FPS), so the physics don't depend on how fast frames are drawn.

- Optional drag can be applied to each mass through its own drag coefficient.

- Every parameter can be edited from the settings screen: time speed multiplier, length of both rods, gravity, both masses, both drag coefficients and the initial angles and angular velocities.

    A small diagram is shown in settings to explain how the angles are measured (clockwise from the horizontal) and what direction counts as a positive angular velocity.

- The current state of the system can be shown live while the simulation is running.

- The window can be resized freely.


## Controls

Everything can be done with the on-screen buttons or with the following shortcuts:
| Key | Action |
|---|---|
| Space | Start, stop or continue the simulation |
| R | Restart the simulation from the initial state |
| S | Open settings |
| B | Go back to the menu from settings |
| E | Show or hide the live stats while running |
| Esc | Unfocus the selected text field in settings |


## Build

The project uses Make to build and is currently configured with the following options:
| Command | Use case |
|---|---|
| make run | To compile and run for desktop. Needs [Raylib](https://www.raylib.com/) and [raygui](https://github.com/raysan5/raygui) installed. Right now it's only configured for Linux although I might expand to other platforms in the future |
| make debug | To compile with debugger friendly flags (no optimizations) |
| make web | To compile to WASM. Needs the [Emscripten](https://emscripten.org/) compiler, an installation of [Raylib](https://www.raylib.com/) built for WASM and the [raygui](https://github.com/raysan5/raygui) header (paths must be provided in the .env file, see .example.env) |
| make serve | To serve the WASM build in a local server using python |


## Status

In development.


## License

Copyright (c) 2026 bentor. All rights reserved.

No license is granted to use, copy, modify or distribute this code.
