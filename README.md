# N-Body Gravity Simulation

A source-first OpenGL experiment for simulating gravitational motion, orbital drift, and space-like deformation in real time.

<div align="center">
  <img src="./Screenshot%202026-06-11%20191826.png" alt="N-body gravity simulation preview" width="960" />
</div>

## Overview

This project explores how a cluster of bodies moves under mutual gravity while a live deformed grid reacts to their mass and motion. The result is a compact simulation that feels more like a visual physics sketchbook than a strict numerical solver.

## Project structure

- `src/main.cpp` — simulation loop, rendering, camera, and orbital behavior
- `src/triangle_test.cpp` — lightweight graphics sanity check
- `CMakeLists.txt` — build configuration
- `README.md` — project overview and screenshots

## Features

- Real-time N-body gravitational motion
- Interactive 3D camera orbit and zoom
- Dynamic spatial field deformation
- Multiple bodies with mass, velocity, radius, and color
- Lightweight OpenGL rendering

## Dependencies

This project is intended to use system-installed graphics libraries instead of vendored binaries.

- C++17
- OpenGL
- GLFW
- GLEW
- GLM

Install them through your preferred package manager or build toolchain before compiling.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Then run the generated executable from the `build` directory.

## Screenshots

<div align="center">
  <img src="./Screenshot%202026-06-10%20214018.png" alt="Simulation view 1" width="480" />
  <img src="./Screenshot%202026-06-11%20191826.png" alt="Simulation view 2" width="480" />
</div>

## Notes

This repository keeps the project focused on the source code in `src/` and leaves dependency resolution to the local environment.