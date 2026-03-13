/*
 * Triple A Hero Shooter - V1 Foundation
 *
 * Architecture:
 *   - Fixed 256-tick physics simulation (3.90625ms per tick)
 *   - Uncapped rendering FPS
 *   - Decoupled physics from rendering (interpolation planned for V2)
 *   - SDL2 + OpenGL 2.1 (compatible, will upgrade later)
 *   - Raw mouse input, no acceleration
 */

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "util/math_utils.h"
#include "engine/renderer.h"
#include "engine/input.h"
#include "engine/audio.h"
#include "game/player.h"
#include "game/arena.h"

// ---- Game configuration ----
#define PHYSICS_TICK_RATE  256
#define PHYSICS_DT         (1.0 / PHYSICS_TICK_RATE)  // ~3.90625ms
#define MAX_PHYSICS_STEPS  8   // Cap to prevent spiral of death

#define WINDOW_TITLE       "Hero Shooter V1"
#define DEFAULT_WIDTH      1920
#define DEFAULT_HEIGHT     1080
#define DEFAULT_FOV        103.0f
#define MIN_FOV            80.0f
#define MAX_FOV            120.0f

// ---- Game state ----
typedef struct {
    Player player;
    Arena  arena;

    // Rendering
    RenderState render_state;
    float fov;

    // Audio
    AudioState audio;

    // Timing
    double physics_accumulator;
    Uint64 last_frame_time;
    Uint64 perf_freq;

    // Performance stats
    float fps;
    float frame_time_ms;
    int   fps_counter;
    float fps_timer;

    // Shot feedback
    float hit_marker_timer;
    Vec3  last_hit_pos;
    int   show_hit_marker;

    // Settings menu
    int settings_open;
    int settings_mouse_prev;  // Previous frame mouse state for click detection

    // Track previous on_ground for land sound
    int prev_on_ground;
} GameState;

static GameState game;

// ---- Forward declarations ----
static void game_init(SDL_Window* window);
static void game_physics_tick(InputState* input);
static void game_render(SDL_Window* window, InputState* input);
static void game_fire_weapon(void);
static void draw_hit_marker(int screen_w, int screen_h);
static void draw_fps_counter(int screen_w, int screen_h);
static void draw_settings_overlay(int screen_w, int screen_h, InputState* input);

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Request OpenGL 2.1 context (maximum compatibility)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Disable VSync for uncapped FPS
    SDL_GL_SetSwapInterval(0);

    // Initialize input
    InputState input;
    input_init(&input, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    // Initialize game
    game_init(window);

    printf("Hero Shooter V1 initialized.\n");
    printf("  Physics: %d tick/s (%.4f ms/tick)\n", PHYSICS_TICK_RATE, PHYSICS_DT * 1000.0);
    printf("  Rendering: Uncapped FPS\n");
    printf("  FOV: %.0f (configurable %.0f-%.0f)\n", DEFAULT_FOV, MIN_FOV, MAX_FOV);
    printf("Controls:\n");
    printf("  WASD    - Move\n");
    printf("  Mouse   - Look\n");
    printf("  LMB     - Fire\n");
    printf("  R       - Reload\n");
    printf("  Space   - Jump\n");
    printf("  Ctrl    - Crouch\n");
    printf("  ESC     - Settings\n");

    // ---- Main loop ----
    int running = 1;
    while (running) {
        input_begin_frame(&input);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            input_process_event(&input, &event);
        }

        if (input.quit_requested) {
            running = 0;
            break;
        }

        // Toggle settings with ESC
        if (input_key_pressed(&input, SDL_SCANCODE_ESCAPE)) {
            game.settings_open = !game.settings_open;
            if (game.settings_open) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
            } else {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
        }

        // Handle window resize
        if (input.window_resized) {
            glViewport(0, 0, input.window_w, input.window_h);
            game.render_state.aspect = (float)input.window_w / (float)input.window_h;
            renderer_set_fov(&game.render_state, game.fov);
        }

        // ---- Fixed timestep physics ----
        Uint64 current_time = SDL_GetPerformanceCounter();
        double frame_dt = (double)(current_time - game.last_frame_time) / (double)game.perf_freq;
        game.last_frame_time = current_time;

        // Clamp frame delta to prevent huge jumps (e.g., after breakpoint)
        if (frame_dt > 0.1) frame_dt = 0.1;

        game.physics_accumulator += frame_dt;

        // Apply mouse look ONCE per frame, before physics ticks.
        // Mouse delta must not be applied per-tick, otherwise sensitivity
        // scales with the number of physics steps per frame.
        if (!game.settings_open) {
            float sens = input.sensitivity;
            float dx = input.mouse_dx;
            float dy = input.mouse_dy;

            // Apply invert settings
            if (input.invert_x) dx = -dx;
            if (input.invert_y) dy = -dy;

            // Default: mouse right = look right, mouse down = look down
            // In our coordinate system: positive yaw = turn left, so we negate dx
            game.player.yaw   -= dx * sens;
            game.player.pitch += dy * sens;
            if (game.player.pitch > 89.0f)  game.player.pitch = 89.0f;
            if (game.player.pitch < -89.0f) game.player.pitch = -89.0f;
            while (game.player.yaw >= 360.0f) game.player.yaw -= 360.0f;
            while (game.player.yaw < 0.0f)    game.player.yaw += 360.0f;
        }

        int physics_steps = 0;
        while (game.physics_accumulator >= PHYSICS_DT && physics_steps < MAX_PHYSICS_STEPS) {
            game_physics_tick(&input);
            game.physics_accumulator -= PHYSICS_DT;
            physics_steps++;
        }

        // If we hit max steps, drain remaining accumulator to prevent spiral
        if (physics_steps >= MAX_PHYSICS_STEPS) {
            game.physics_accumulator = 0;
        }

        // ---- Render ----
        game_render(window, &input);

        // ---- FPS tracking ----
        game.fps_counter++;
        game.fps_timer += (float)frame_dt;
        game.frame_time_ms = (float)(frame_dt * 1000.0);

        if (game.fps_timer >= 0.5f) {
            game.fps = game.fps_counter / game.fps_timer;
            game.fps_counter = 0;
            game.fps_timer = 0;
        }

        SDL_GL_SwapWindow(window);
    }

    audio_shutdown(&game.audio);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

static void game_init(SDL_Window* window) {
    memset(&game, 0, sizeof(game));

    game.fov = DEFAULT_FOV;
    game.perf_freq = SDL_GetPerformanceFrequency();
    game.last_frame_time = SDL_GetPerformanceCounter();

    // Initialize renderer
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    renderer_init(&game.render_state, game.fov, (float)w / (float)h);

    // Initialize audio
    audio_init(&game.audio);

    // Initialize player at arena center
    player_init(&game.player, vec3(0, PLAYER_EYE_HEIGHT, 5.0f));

    // Initialize arena
    arena_init(&game.arena);

    game.prev_on_ground = 1;
}

static void game_physics_tick(InputState* input) {
    float dt = (float)PHYSICS_DT;

    // Update player (movement + aim)
    if (!game.settings_open) {
        player_update(&game.player, input, dt);
    }

    // Resolve collisions with arena
    game.player.position = arena_resolve_collision(
        &game.arena, game.player.position, PLAYER_RADIUS);

    // Handle firing
    if (!game.settings_open &&
        input_mouse_held(input, SDL_BUTTON_LEFT) &&
        game.player.weapon_state != WEAPON_RELOADING &&
        game.player.fire_cooldown <= 0 &&
        game.player.ammo > 0) {
        game_fire_weapon();
    }

    // ---- Audio: footsteps ----
    audio_update_footsteps(&game.audio,
        game.player.is_walking, game.player.is_crouching, dt);

    // ---- Audio: landing sound ----
    if (game.player.on_ground && !game.prev_on_ground) {
        audio_play_sound(&game.audio, SOUND_LAND);
    }
    game.prev_on_ground = game.player.on_ground;

    // ---- Audio: jump sound ----
    if (!game.settings_open &&
        input_key_pressed(input, SDL_SCANCODE_SPACE) &&
        game.prev_on_ground) {
        audio_play_sound(&game.audio, SOUND_JUMP);
    }

    // ---- Audio: reload sound ----
    if (game.player.weapon_state == WEAPON_RELOADING &&
        game.player.reload_timer > WEAPON_RELOAD_TIME - (float)PHYSICS_DT * 1.5f) {
        audio_play_sound(&game.audio, SOUND_RELOAD);
    }

    // Update arena (target respawns, etc)
    arena_update(&game.arena, dt);

    // Update hit marker
    if (game.hit_marker_timer > 0) {
        game.hit_marker_timer -= dt;
        if (game.hit_marker_timer <= 0) {
            game.show_hit_marker = 0;
        }
    }
}

static void game_fire_weapon(void) {
    game.player.ammo--;
    game.player.fire_cooldown = 1.0f / WEAPON_FIRE_RATE;
    game.player.shots_fired++;

    // Gun recoil animation
    game.player.gun_recoil = 1.0f;

    // Gunshot sound
    audio_play_sound(&game.audio, SOUND_GUNSHOT);

    // Hitscan raycast
    Ray aim_ray = player_get_aim_ray(&game.player);
    float hit_dist = 0;
    int hit_target = arena_raycast_targets(&game.arena, aim_ray, WEAPON_RANGE, &hit_dist);

    if (hit_target >= 0) {
        Target* t = &game.arena.targets[hit_target];
        t->health -= WEAPON_DAMAGE;
        t->hit_flash = 1.0f;
        game.player.shots_hit++;

        // Hit marker feedback
        game.show_hit_marker = 1;
        game.hit_marker_timer = 0.15f;

        if (t->health <= 0) {
            t->alive = 0;
            t->respawn_timer = 3.0f;
            game.hit_marker_timer = 0.3f;
        }

        game.last_hit_pos = vec3_add(aim_ray.origin,
            vec3_scale(aim_ray.direction, hit_dist));
    }
}

static void game_render(SDL_Window* window, InputState* input) {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    // Set view matrix from player camera
    Mat4 view = player_get_view_matrix(&game.player);
    renderer_set_view(&game.render_state, view);

    // Begin rendering
    renderer_begin_frame(&game.render_state);

    // Render arena (floor, targets, walls)
    arena_render(&game.arena, &game.render_state);

    // Draw bullet trace line (brief visual feedback)
    if (game.show_hit_marker && game.hit_marker_timer > 0.1f) {
        Ray aim = player_get_aim_ray(&game.player);
        Vec3 trace_end = vec3_add(aim.origin, vec3_scale(aim.direction, 2.0f));
        renderer_draw_line(&game.render_state, aim.origin, trace_end, 1.0f, 1.0f, 0.5f);
    }

    // ---- 2D HUD ----
    // Gun viewmodel (behind crosshair)
    renderer_draw_gun_viewmodel(w, h,
        game.player.gun_recoil,
        game.player.gun_bob_timer,
        game.player.weapon_state == WEAPON_RELOADING);

    renderer_draw_crosshair(w, h);
    renderer_draw_hud(w, h, game.player.health, PLAYER_MAX_HEALTH, game.player.ammo, game.player.max_ammo);

    // Hit marker
    if (game.show_hit_marker) {
        draw_hit_marker(w, h);
    }

    // FPS counter
    draw_fps_counter(w, h);

    // Accuracy display
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);

        char acc_text[64];
        float accuracy = game.player.shots_fired > 0 ?
            (float)game.player.shots_hit / game.player.shots_fired * 100.0f : 0.0f;
        snprintf(acc_text, sizeof(acc_text), "ACC: %.0f%%", accuracy);
        renderer_draw_text_simple(w - 120.0f, h - 60.0f, acc_text, 0.7f, 0.7f, 0.7f);

        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    // Settings overlay
    if (game.settings_open) {
        draw_settings_overlay(w, h, input);
    }
}

static void draw_hit_marker(int screen_w, int screen_h) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screen_w, screen_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    float cx = screen_w * 0.5f;
    float cy = screen_h * 0.5f;
    float size = 10.0f;
    float gap = 4.0f;
    float alpha = game.hit_marker_timer * 6.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    // Red X marker
    glColor4f(1.0f, 0.2f, 0.2f, alpha);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(cx - size, cy - size);
    glVertex2f(cx - gap,  cy - gap);
    glVertex2f(cx + gap,  cy - gap);
    glVertex2f(cx + size, cy - size);
    glVertex2f(cx - size, cy + size);
    glVertex2f(cx - gap,  cy + gap);
    glVertex2f(cx + gap,  cy + gap);
    glVertex2f(cx + size, cy + size);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void draw_fps_counter(int screen_w, int screen_h) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screen_w, screen_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    char fps_text[64];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", game.fps);
    renderer_draw_text_simple(10.0f, 10.0f, fps_text, 0.0f, 1.0f, 0.0f);

    char ms_text[64];
    snprintf(ms_text, sizeof(ms_text), "%.1fMS", game.frame_time_ms);
    renderer_draw_text_simple(10.0f, 30.0f, ms_text, 0.7f, 0.7f, 0.7f);

    char tick_text[64];
    snprintf(tick_text, sizeof(tick_text), "TICK: %d", PHYSICS_TICK_RATE);
    renderer_draw_text_simple(10.0f, 50.0f, tick_text, 0.7f, 0.7f, 0.7f);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void draw_settings_overlay(int screen_w, int screen_h, InputState* input) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screen_w, screen_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    // Darken background
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glVertex2f(0, 0);
    glVertex2f(screen_w, 0);
    glVertex2f(screen_w, screen_h);
    glVertex2f(0, screen_h);
    glEnd();

    float panel_x = screen_w * 0.25f;
    float panel_y = screen_h * 0.1f;
    float panel_w = screen_w * 0.5f;
    float panel_h = 480.0f;

    // Panel background
    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.15f, 0.2f, 0.95f);
    glVertex2f(panel_x, panel_y);
    glVertex2f(panel_x + panel_w, panel_y);
    glVertex2f(panel_x + panel_w, panel_y + panel_h);
    glVertex2f(panel_x, panel_y + panel_h);
    glEnd();

    renderer_draw_text_simple(panel_x + 20, panel_y + 20, "SETTINGS", 1.0f, 1.0f, 1.0f);

    // ---- FOV slider ----
    renderer_draw_text_simple(panel_x + 20, panel_y + 60, "FOV:", 0.8f, 0.8f, 0.8f);
    char fov_text[32];
    snprintf(fov_text, sizeof(fov_text), "%.0f", game.fov);
    renderer_draw_text_simple(panel_x + 80, panel_y + 60, fov_text, 1.0f, 1.0f, 0.5f);

    float slider_x = panel_x + 20;
    float slider_y = panel_y + 90;
    float slider_w = panel_w - 40;
    float slider_h = 10;

    glBegin(GL_QUADS);
    glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
    glVertex2f(slider_x, slider_y);
    glVertex2f(slider_x + slider_w, slider_y);
    glVertex2f(slider_x + slider_w, slider_y + slider_h);
    glVertex2f(slider_x, slider_y + slider_h);
    glEnd();

    float fov_frac = (game.fov - MIN_FOV) / (MAX_FOV - MIN_FOV);
    float knob_x = slider_x + fov_frac * slider_w;

    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.7f, 0.2f, 1.0f);
    glVertex2f(knob_x - 5, slider_y - 3);
    glVertex2f(knob_x + 5, slider_y - 3);
    glVertex2f(knob_x + 5, slider_y + slider_h + 3);
    glVertex2f(knob_x - 5, slider_y + slider_h + 3);
    glEnd();

    int mx, my;
    Uint32 mb = SDL_GetMouseState(&mx, &my);
    if (mb & SDL_BUTTON(1)) {
        if (mx >= slider_x && mx <= slider_x + slider_w &&
            my >= slider_y - 10 && my <= slider_y + slider_h + 10) {
            float new_frac = (mx - slider_x) / slider_w;
            game.fov = MIN_FOV + new_frac * (MAX_FOV - MIN_FOV);
            game.fov = clampf(game.fov, MIN_FOV, MAX_FOV);
            renderer_set_fov(&game.render_state, game.fov);
        }
    }

    // ---- Sensitivity slider ----
    renderer_draw_text_simple(panel_x + 20, panel_y + 130, "SENS:", 0.8f, 0.8f, 0.8f);
    char sens_text[32];
    snprintf(sens_text, sizeof(sens_text), "%.2f", input->sensitivity);
    renderer_draw_text_simple(panel_x + 90, panel_y + 130, sens_text, 1.0f, 1.0f, 0.5f);

    float sens_slider_y = panel_y + 160;
    glBegin(GL_QUADS);
    glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
    glVertex2f(slider_x, sens_slider_y);
    glVertex2f(slider_x + slider_w, sens_slider_y);
    glVertex2f(slider_x + slider_w, sens_slider_y + slider_h);
    glVertex2f(slider_x, sens_slider_y + slider_h);
    glEnd();

    float sens_frac = (input->sensitivity - 0.01f) / (0.5f - 0.01f);
    float sens_knob_x = slider_x + clampf(sens_frac, 0, 1) * slider_w;

    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.7f, 0.2f, 1.0f);
    glVertex2f(sens_knob_x - 5, sens_slider_y - 3);
    glVertex2f(sens_knob_x + 5, sens_slider_y - 3);
    glVertex2f(sens_knob_x + 5, sens_slider_y + slider_h + 3);
    glVertex2f(sens_knob_x - 5, sens_slider_y + slider_h + 3);
    glEnd();

    if (mb & SDL_BUTTON(1)) {
        if (mx >= slider_x && mx <= slider_x + slider_w &&
            my >= sens_slider_y - 10 && my <= sens_slider_y + slider_h + 10) {
            float new_frac = (mx - slider_x) / slider_w;
            input->sensitivity = 0.01f + new_frac * (0.5f - 0.01f);
            input->sensitivity = clampf(input->sensitivity, 0.01f, 0.5f);
        }
    }

    // ---- Mouse direction toggles ----
    float toggle_y = panel_y + 210;
    int mouse_just_pressed = (mb & SDL_BUTTON(1)) && !(game.settings_mouse_prev & SDL_BUTTON(1));

    // Invert X toggle
    renderer_draw_text_simple(panel_x + 20, toggle_y, "INVERT X:", 0.8f, 0.8f, 0.8f);
    {
        float btn_x = panel_x + 140;
        float btn_w = 60;
        float btn_h = 18;

        glBegin(GL_QUADS);
        if (input->invert_x) {
            glColor4f(1.0f, 0.5f, 0.2f, 0.9f);
        } else {
            glColor4f(0.3f, 0.3f, 0.35f, 0.9f);
        }
        glVertex2f(btn_x, toggle_y - 2);
        glVertex2f(btn_x + btn_w, toggle_y - 2);
        glVertex2f(btn_x + btn_w, toggle_y + btn_h);
        glVertex2f(btn_x, toggle_y + btn_h);
        glEnd();

        renderer_draw_text_simple(btn_x + 10, toggle_y + 1,
            input->invert_x ? "ON" : "OFF",
            1.0f, 1.0f, 1.0f);

        if (mouse_just_pressed &&
            mx >= btn_x && mx <= btn_x + btn_w &&
            my >= toggle_y - 2 && my <= toggle_y + btn_h) {
            input->invert_x = !input->invert_x;
        }
    }

    // Invert Y toggle
    float toggle_y2 = toggle_y + 35;
    renderer_draw_text_simple(panel_x + 20, toggle_y2, "INVERT Y:", 0.8f, 0.8f, 0.8f);
    {
        float btn_x = panel_x + 140;
        float btn_w = 60;
        float btn_h = 18;

        glBegin(GL_QUADS);
        if (input->invert_y) {
            glColor4f(1.0f, 0.5f, 0.2f, 0.9f);
        } else {
            glColor4f(0.3f, 0.3f, 0.35f, 0.9f);
        }
        glVertex2f(btn_x, toggle_y2 - 2);
        glVertex2f(btn_x + btn_w, toggle_y2 - 2);
        glVertex2f(btn_x + btn_w, toggle_y2 + btn_h);
        glVertex2f(btn_x, toggle_y2 + btn_h);
        glEnd();

        renderer_draw_text_simple(btn_x + 10, toggle_y2 + 1,
            input->invert_y ? "ON" : "OFF",
            1.0f, 1.0f, 1.0f);

        if (mouse_just_pressed &&
            mx >= btn_x && mx <= btn_x + btn_w &&
            my >= toggle_y2 - 2 && my <= toggle_y2 + btn_h) {
            input->invert_y = !input->invert_y;
        }
    }

    // Update previous mouse state for next frame
    game.settings_mouse_prev = mb;

    // ---- Volume slider ----
    float vol_y = toggle_y2 + 45;
    renderer_draw_text_simple(panel_x + 20, vol_y, "VOLUME:", 0.8f, 0.8f, 0.8f);
    char vol_text[32];
    snprintf(vol_text, sizeof(vol_text), "%.0f%%", game.audio.master_volume * 100.0f);
    renderer_draw_text_simple(panel_x + 110, vol_y, vol_text, 1.0f, 1.0f, 0.5f);

    float vol_slider_y = vol_y + 30;
    glBegin(GL_QUADS);
    glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
    glVertex2f(slider_x, vol_slider_y);
    glVertex2f(slider_x + slider_w, vol_slider_y);
    glVertex2f(slider_x + slider_w, vol_slider_y + slider_h);
    glVertex2f(slider_x, vol_slider_y + slider_h);
    glEnd();

    float vol_frac = game.audio.master_volume;
    float vol_knob_x = slider_x + clampf(vol_frac, 0, 1) * slider_w;

    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.7f, 0.2f, 1.0f);
    glVertex2f(vol_knob_x - 5, vol_slider_y - 3);
    glVertex2f(vol_knob_x + 5, vol_slider_y - 3);
    glVertex2f(vol_knob_x + 5, vol_slider_y + slider_h + 3);
    glVertex2f(vol_knob_x - 5, vol_slider_y + slider_h + 3);
    glEnd();

    if (mb & SDL_BUTTON(1)) {
        if (mx >= slider_x && mx <= slider_x + slider_w &&
            my >= vol_slider_y - 10 && my <= vol_slider_y + slider_h + 10) {
            float new_frac = (mx - slider_x) / slider_w;
            game.audio.master_volume = clampf(new_frac, 0.0f, 1.0f);
        }
    }

    // ---- Instructions ----
    renderer_draw_text_simple(panel_x + 20, panel_y + panel_h - 50,
        "ESC TO CLOSE", 0.5f, 0.5f, 0.5f);
    renderer_draw_text_simple(panel_x + 20, panel_y + panel_h - 25,
        "ALT+F4 TO QUIT", 0.5f, 0.5f, 0.5f);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
