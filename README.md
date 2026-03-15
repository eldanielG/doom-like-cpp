# doom-like-cpp

**A modular retro FPS prototype built with C++17, raylib, and CMake.**

`doom-like-cpp` is a focused first-person shooter prototype inspired by classic
90s FPS design, built around raycasting, fast iteration, and controlled scope.
The goal is to explore gameplay feel, combat systems, and clean technical
structure without prematurely expanding into a full engine or a full DOOM clone.

> Current status: **first playable combat build**

## Features

- Raycasting-based pseudo-3D rendering with wall shading and a retro visual style
- Player movement, collision, shooting, health, damage feedback, and game over flow
- Enemy behavior with patrol, chase, line-of-sight checks, and short-term pursuit memory
- Simple ranged enemy attacks with readable projectile feedback
- Score, best score, survival time, and best time tracking during the session
- Progressive difficulty scaling based on survival time
- Integrated gameplay audio for shooting, hits, player damage, and game over
- Modular architecture split into `core`, `entities`, `render`, and `world`
- HUD, crosshair, minimap, and a lightweight first-person weapon presentation

## Tech Stack

- `C++17`
- `raylib`
- `CMake`
- Custom raycasting renderer
- No full game engine

## Controls

| Action | Input |
| --- | --- |
| Move | `W`, `A`, `S`, `D` |
| Turn camera | `Left Arrow`, `Right Arrow` |
| Fire | `Mouse Left`, `Space` |
| Reset run | `R` |
| Quit | `Esc` / window close |

## Build and Run

If `raylib` is available on your system, CMake will use it. Otherwise, the
project fetches `raylib` automatically during configuration.

```powershell
cmake -S . -B build
cmake --build build
```

Run the generated `doom_like` executable from your build output directory.
On Visual Studio generators, this is typically something like:

```powershell
.\build\Debug\doom_like.exe
```

## Project Structure

```text
doom-like-cpp/
|-- include/
|   |-- core/       # game state, flow, audio
|   |-- entities/   # player and enemy logic
|   |-- render/     # raycasting, HUD, minimap, weapon, sprites
|   `-- world/      # map, collision, spawn points
|-- src/
|   |-- core/
|   |-- entities/
|   |-- render/
|   |-- world/
|   `-- main.cpp    # bootstrap and main loop
|-- assets/
`-- CMakeLists.txt
```

## Roadmap

Short-term next steps are centered on improving gameplay depth without losing
the project's lightweight structure:

- combat feel and encounter polish
- richer visual presentation
- more iteration on arena flow and challenge pacing

## Project Docs

- Version history and release notes: [CHANGELOG.md](./CHANGELOG.md)
- Audio attribution and asset notes: [CREDITS.md](./CREDITS.md)
