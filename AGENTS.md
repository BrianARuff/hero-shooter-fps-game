# AGENTS.md — Contributor Guide for AI Agents and Developers

This document defines the recurring patterns, conventions, and architecture decisions used in this project. Any AI agent (Claude, GPT, Copilot, Gemini, etc.) or human contributor should read this file before making changes to ensure consistency.

---

## Project Overview

A first-person hero shooter built from scratch in C with SDL2 and OpenGL. The goal is to build a AAA-quality foundation with pinpoint-accurate physics and maximum performance, then layer artwork and features on top.

**Genre:** Hero Shooter (Overwatch-style)
**Language:** C (C99 standard, compiled as C)
**Graphics:** SDL2 for windowing/input + OpenGL 2.1 for rendering
**Target Platform:** Windows (64-bit), cross-compiled from Linux via Zig
**Design Philosophy:** Movement does not affect aim. Physics must be frame-rate independent.

---

## Build System

### Cross-Compilation Toolchain

The project is cross-compiled from Linux to Windows using:

- **Zig 0.11.0** as the C cross-compiler (`zig cc -target x86_64-windows-gnu`)
- **SDL2 2.30.1** pre-built MinGW development libraries
- Output is a Windows PE32+ `.exe` plus `SDL2.dll`

### Build Command

```bash
bash build.sh
```

This compiles all `.c` files, links against SDL2 and OpenGL, and outputs `HeroShooter.exe` + `SDL2.dll` into the `build/` directory.

### Adding a New Source File

1. Create the `.h` and `.c` files in the appropriate directory (see Project Structure below).
2. Add the `.c` file to the `$ZIG cc` command in `build.sh`.
3. Rebuild with `bash build.sh`.

There is no CMake or Makefile — the build is a single shell script for simplicity. This may change as the project grows.

**Note:** `build.sh` contains hardcoded paths to the Zig compiler and SDL2 libraries specific to the original build environment (a Claude Cowork Linux VM). If building in a different environment, you will need to update the `ZIG`, `SDL2`, `SRC`, and `OUT` variables at the top of `build.sh` to point to your local installations. Alternatively, install Zig and SDL2 dev libraries via your system package manager and adjust paths accordingly.

---

## Project Structure

```
game_v1/
├── build.sh                 # Build script (cross-compiles to Windows)
├── HeroShooter.exe          # Compiled game binary (Windows)
├── SDL2.dll                 # Required runtime dependency
├── AGENTS.md                # This file
├── README.md                # Feature list and changelog
└── src/
    ├── main.c               # Entry point, game loop, HUD, settings
    ├── engine/
    │   ├── input.h / input.c       # SDL2 input handling, raw mouse
    │   ├── renderer.h / renderer.c # OpenGL rendering, text, HUD, gun viewmodel
    │   └── audio.h / audio.c       # Procedural audio system (SDL2 audio API)
    ├── game/
    │   ├── player.h / player.c     # Player entity, movement, jump, crouch, weapon
    │   ├── arena.h / arena.c       # Test arena, targets, walls, collision
    └── util/
        └── math_utils.h            # Vec3, Mat4, AABB, Ray, math helpers
```

### Directory Conventions

| Directory | Purpose |
|-----------|---------|
| `src/engine/` | Platform and rendering systems. No game logic. |
| `src/game/` | Game-specific entities, mechanics, and world. |
| `src/util/` | Pure math and data structure utilities. No dependencies on SDL2 or OpenGL. |

### File Naming

- All files use `snake_case.c` / `snake_case.h`.
- Every `.c` file has a corresponding `.h` header.
- Header-only files (like `math_utils.h`) use `static inline` functions.
- All headers use include guards: `#ifndef FILENAME_H` / `#define FILENAME_H`.

---

## Architecture Patterns

### Game Loop (Fixed Timestep Physics + Uncapped Rendering)

The core loop in `main.c` follows this pattern:

```
while running:
    poll SDL events
    accumulate frame delta time
    while accumulator >= PHYSICS_DT:
        run one physics tick (1/256 second)
        drain accumulator
    render frame (uncapped)
    swap buffers
```

**Critical rules:**
- Physics runs at exactly 256 ticks/second (`PHYSICS_DT = 1.0/256`).
- Rendering is completely decoupled and runs as fast as possible.
- The accumulator is capped at `MAX_PHYSICS_STEPS` (8) to prevent a spiral of death.
- All gameplay logic (movement, shooting, collision) happens in the physics tick, never in the render loop.

### Movement System

- Player movement uses acceleration + friction model (not direct velocity setting).
- Diagonal movement is normalized to prevent speed boost.
- Movement direction is computed from camera yaw only (pitch does not affect movement).
- **Movement does not affect aim.** Mouse input is applied independently of movement state.
- **Jump:** Space bar triggers 7.5 m/s upward velocity. Gravity at 20 m/s². Ground detection at y=0.
- **Crouch:** Ctrl key held to crouch. Eye height smoothly interpolates to 1.0m (from 1.6m). Walk speed halved. Collision height reduced.
- `player->foot_y` tracks the player's feet position (0 = on ground). `player->position.y` = foot_y + current_eye_height.

### Input System

- Raw mouse input via `SDL_SetRelativeMouseMode(SDL_TRUE)`.
- No mouse acceleration or smoothing — 1:1 mouse delta to aim rotation.
- Keyboard uses SDL scancodes (hardware position, not layout-dependent).
- Input state tracks current + previous frame for pressed/released detection.
- **CRITICAL: Mouse look is applied ONCE per frame in the main loop, NOT inside the physics tick.** If mouse look were inside `player_update()`, sensitivity would scale with the number of physics ticks per frame (which varies). This was a bug found and fixed during QC.
- Movement input (WASD) is safe to read inside physics ticks because it's a binary on/off state, not an accumulated delta.
- **Mouse direction:** Default is mouse-right = look-right. `yaw -= mouse_dx * sensitivity` (negated because positive yaw = turn left in our coordinate system). Invert X/Y toggles available in settings.
- ESC opens/closes the settings menu. Game quit is via Alt+F4 or SDL_QUIT event.

### Audio System

- Uses SDL2's built-in audio API (`SDL_OpenAudioDevice`) with a callback-based architecture.
- All sounds are procedurally generated (no external audio files needed).
- 16-voice polyphonic mixing with voice stealing (oldest voice replaced when full).
- Audio callback runs on a separate thread — voice state modifications are wrapped in `SDL_LockAudioDevice`/`SDL_UnlockAudioDevice`.
- Sound types: footstep (thud), jump (rising sweep), land (impact), gunshot (crack+thump), reload (clicks).
- Footstep timing is managed by `audio_update_footsteps()` called from the physics tick.

### Rendering Approach

- Currently uses OpenGL 2.1 immediate mode (`glBegin`/`glEnd`) for rapid prototyping.
- This WILL be replaced with modern OpenGL (VAOs/VBOs/shaders) or Vulkan later.
- All 3D rendering goes through `RenderState` which holds projection and view matrices.
- HUD rendering switches to orthographic projection temporarily.
- Text is drawn with a minimal line-segment font (no texture dependencies).

### Collision System

- Uses AABB (Axis-Aligned Bounding Boxes) for all collisions.
- Player collision uses a cylinder approximated as AABB on XZ plane + radius.
- Wall collision resolves by pushing the player out along the shortest axis.
- Hitscan uses ray-AABB intersection tests.

### Weapon System

- Hitscan weapons fire an instant raycast from the camera position along the look direction.
- Fire rate is controlled by a cooldown timer decremented each physics tick.
- Reload is time-based with automatic reload on empty magazine.
- Hit detection returns the closest target hit and the distance.

---

## Coding Conventions

### C Style

- **C99 standard.** No C++ features.
- Use `typedef struct { ... } TypeName;` for all structs.
- Constants defined as `#define` in the relevant header.
- Function naming: `module_action_noun()` (e.g., `player_get_forward()`, `arena_raycast_targets()`).
- Boolean returns: `1` = true, `0` = false (no `stdbool.h` currently).
- Avoid heap allocation where possible. Use fixed-size arrays with `#define MAX_*` limits.

### Memory Management

- No dynamic memory allocation in the game loop (no `malloc`/`free` during gameplay).
- All entities use fixed-size arrays defined at compile time.
- Limits: `MAX_TARGETS = 32`, `MAX_WALLS = 64`. Increase as needed.

### Error Handling

- SDL errors: print to stderr and exit with non-zero code.
- Game logic: fail silently with bounds checks (e.g., `if (num_targets >= MAX_TARGETS) return;`).

---

## Windows Cross-Compilation Notes

### Reserved Keywords

Windows headers (`windef.h`) define `near` and `far` as macros. **Never use these as variable names.** Use `near_plane` / `far_plane` instead. This applies to any identifier that might conflict with Win32 macros.

### Linking

The following Windows system libraries are needed and specified in `build.sh`:
`-lopengl32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lsetupapi -lversion -lshell32 -luser32`

### SDL2 Setup

`SDL2.dll` must be in the same directory as the `.exe`. The build script copies it automatically.

Always use `#define SDL_MAIN_HANDLED` and call `SDL_SetMainReady()` before `SDL_Init()` to avoid SDL's main macro hijacking.

---

## Adding New Features — Checklist

When adding a new feature, follow these steps:

1. **Determine which directory** the code belongs in (`engine/`, `game/`, or `util/`).
2. **Create `.h` and `.c` files** following the naming conventions above.
3. **Add the `.c` file** to the build command in `build.sh`.
4. **Keep physics in the physics tick.** Any gameplay logic must run at 256Hz in `game_physics_tick()`.
5. **Keep rendering in the render function.** Drawing code goes in `game_render()` or a function it calls.
6. **Test the build** by running `bash build.sh` and verifying the `.exe` runs.
7. **Update README.md** with the new feature in the feature list.
8. **Update this file** if you introduce a new pattern or convention.

---

## Game Constants Reference

| Constant | Value | Location |
|----------|-------|----------|
| Physics tick rate | 256 Hz | `main.c` |
| Max physics steps per frame | 8 | `main.c` |
| Default FOV | 103 degrees | `main.c` |
| FOV range | 80 - 120 degrees | `main.c` |
| Player walk speed | 5.5 m/s | `player.h` |
| Player crouch speed | 2.75 m/s | `player.h` |
| Player height | 1.8 m | `player.h` |
| Player crouch height | 1.2 m | `player.h` |
| Player eye height | 1.6 m | `player.h` |
| Player crouch eye height | 1.0 m | `player.h` |
| Player collision radius | 0.3 m | `player.h` |
| Player max health | 200 HP | `player.h` |
| Player jump speed | 7.5 m/s | `player.h` |
| Player gravity | 20.0 m/s² | `player.h` |
| Weapon damage | 19 per bullet | `player.h` |
| Weapon fire rate | 9 rounds/sec | `player.h` |
| Weapon magazine size | 25 rounds | `player.h` |
| Weapon reload time | 1.5 seconds | `player.h` |
| Weapon range | 100 m | `player.h` |
| Target respawn time | 3 seconds | `main.c` |
| Mouse sensitivity default | 0.15 | `input.c` |

---

## Future Plans (Architectural Notes)

These are planned features that may affect how current code is written:

- **Multiplayer (client-server):** The physics tick is designed to run identically on client and server. The fixed 256Hz rate will serve as the server tick rate. Input will be serialized and sent to the server. Rendering will use interpolation between physics states.
- **Hero abilities:** The player struct will gain an ability system with cooldowns. Each hero type will be a data definition, not a separate class.
- **Modern renderer:** OpenGL 2.1 immediate mode will be replaced with OpenGL 3.3+ core profile (VAOs, VBOs, shaders) or Vulkan. The `RenderState` abstraction exists to make this transition easier.
- **Jump/Crouch/Sprint:** Movement abilities will be added to the physics tick. Gravity will use the same fixed timestep.
- **Audio:** SDL2_mixer or a custom audio system for weapon sounds, hit confirms, etc.
