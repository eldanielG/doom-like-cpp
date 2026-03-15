# Changelog

All notable changes to this project are documented in this file.

## [v0.1.0] - 2026-03-15

### First Playable Combat Build

#### Added
- Retro FPS prototype foundation built in C++ with raylib and a raycasting-based renderer.
- Pseudo-3D world rendering with wall shading, minimap support, HUD, weapon sprite, and crosshair.
- Player movement, collision, shooting, health, damage feedback, invulnerability window, and game over flow.
- Static targets with patrol behavior, short-range pursuit, short-term chase memory, ranged attacks, hit reaction, elimination, and timed respawn.
- Session-based score, best score, survival time, best time, and progressive difficulty scaling.
- Integrated gameplay sound effects for shooting, target hits, player damage, and game over.

#### Gameplay
- Raycasting-driven arena navigation with responsive movement and combat feedback.
- Enemy behavior that blends patrol, line-of-sight pursuit, short memory after losing sight, and simple ranged pressure.
- Progressive challenge through target speed scaling and respawn pressure over survival time.

#### Architecture
- Reorganized codebase into focused modules for `core`, `entities`, `render`, and `world`.
- Reduced `main.cpp` to initialization and main loop responsibilities.

#### Notes
- Audio credits and asset status are documented in [CREDITS.md](./CREDITS.md).
