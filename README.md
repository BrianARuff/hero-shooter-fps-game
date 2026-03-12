# Hero Shooter — FPS Game

A first-person hero shooter built from scratch in C with SDL2 and OpenGL. Designed as a AAA-quality foundation with accurate physics and maximum performance.

For contributor and AI agent guidelines, see [AGENTS.md](AGENTS.md).

---

## How to Play

1. Ensure `SDL2.dll` is in the same folder as `HeroShooter.exe`.
2. Double-click `HeroShooter.exe`.
3. Play!

### Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move |
| Mouse | Look |
| Left Click | Fire |
| R | Reload |
| F1 | Open/close settings |
| ESC | Quit |

---

## Features (in order added)

### V1 — Physics Foundation (2026-03-12)

1. **256-tick fixed physics simulation** — Physics runs at exactly 256 Hz, decoupled from rendering. Frame rate does not affect gameplay. Accumulator-based timestep with spiral-of-death protection (max 8 physics steps per frame).

2. **Uncapped FPS rendering** — VSync disabled. Rendering runs as fast as the hardware allows. FPS and frame time displayed in the HUD.

3. **Raw mouse input** — SDL2 relative mouse mode with no OS acceleration or smoothing. 1:1 mouse movement to aim. Movement does not affect aim.

4. **Soldier: 76-style player movement** — WASD movement at 5.5 m/s with acceleration/friction model. Diagonal movement normalized (no speed boost). Ground-only movement in V1.

5. **First-person camera** — Yaw/pitch camera with 89-degree pitch clamp. Default 103 FOV (Overwatch default).

6. **Hitscan weapon system** — Instant raycast firing at 9 rounds/sec with 25-round magazine. Ray-AABB intersection for hit detection. 19 damage per bullet. 1.5-second reload with auto-reload on empty.

7. **Test arena** — 80m x 80m flat arena with grid floor. 11 static targets at 5m, 10m, 20m, and 40m. Perimeter walls, interior cover walls, and a raised platform. Distance markers on the floor.

8. **Target system** — Targets display health bars, flash red on hit, change color as health decreases (green to red). Respawn 3 seconds after being destroyed.

9. **AABB collision system** — Player-wall and player-boundary collision with push-out resolution. Cylinder-approximated player collider on the XZ plane.

10. **Basic HUD** — Crosshair (white lines with black outline + center dot), health bar with color coding, ammo counter, accuracy percentage, FPS counter, frame time, and tick rate display.

11. **Hit markers** — Red X flash on hit, longer flash on kill. Visual feedback for successful shots.

12. **Settings menu** — F1 toggles an in-game settings panel with clickable FOV slider (80-120) and mouse sensitivity slider (0.01-0.50). Mouse cursor released when settings are open.

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C (C99) |
| Windowing / Input | SDL2 2.30.1 |
| Rendering | OpenGL 2.1 |
| Build | Zig 0.11.0 (cross-compiler) |
| Target | Windows 64-bit |

---

## Building from Source

The game is cross-compiled from Linux to Windows:

```bash
bash build.sh
```

Output goes to the `build/` directory. See [AGENTS.md](AGENTS.md) for full build details.
