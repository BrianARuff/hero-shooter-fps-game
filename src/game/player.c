#include "player.h"
#include <math.h>

void player_init(Player* player, Vec3 spawn_pos) {
    player->position = spawn_pos;
    player->velocity = vec3(0, 0, 0);
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    player->health = PLAYER_MAX_HEALTH;
    player->alive = 1;

    // Movement
    player->on_ground = 1;
    player->is_crouching = 0;
    player->foot_y = 0.0f;
    player->current_eye_height = PLAYER_EYE_HEIGHT;

    // Weapon
    player->weapon_state = WEAPON_READY;
    player->ammo = WEAPON_MAG_SIZE;
    player->max_ammo = WEAPON_MAG_SIZE;
    player->fire_cooldown = 0.0f;
    player->reload_timer = 0.0f;

    // Gun viewmodel
    player->gun_recoil = 0.0f;
    player->gun_bob_timer = 0.0f;
    player->is_walking = 0;

    player->shots_fired = 0;
    player->shots_hit = 0;

    player_update_bounds(player);
}

Vec3 player_get_forward(Player* player) {
    float yaw_rad = DEG2RAD(player->yaw);
    return vec3(
        -sinf(yaw_rad),
        0.0f,
        -cosf(yaw_rad)
    );
}

Vec3 player_get_right(Player* player) {
    float yaw_rad = DEG2RAD(player->yaw);
    return vec3(
        cosf(yaw_rad),
        0.0f,
        -sinf(yaw_rad)
    );
}

void player_update(Player* player, InputState* input, float dt) {
    if (!player->alive) return;

    // NOTE: Mouse look is applied ONCE per frame in the main loop (main.c),
    // NOT per physics tick. This prevents sensitivity from scaling with tick count.

    // ---- Crouch ----
    player->is_crouching = input_key_held(input, SDL_SCANCODE_LCTRL) ||
                           input_key_held(input, SDL_SCANCODE_RCTRL);

    // ---- Movement ----
    Vec3 forward = player_get_forward(player);
    Vec3 right   = player_get_right(player);

    Vec3 wish_dir = vec3(0, 0, 0);
    if (input_key_held(input, SDL_SCANCODE_W)) wish_dir = vec3_add(wish_dir, forward);
    if (input_key_held(input, SDL_SCANCODE_S)) wish_dir = vec3_sub(wish_dir, forward);
    if (input_key_held(input, SDL_SCANCODE_D)) wish_dir = vec3_add(wish_dir, right);
    if (input_key_held(input, SDL_SCANCODE_A)) wish_dir = vec3_sub(wish_dir, right);

    // Normalize wish direction (prevent diagonal speed boost)
    float wish_len = vec3_length(wish_dir);
    if (wish_len > 0.001f) {
        wish_dir = vec3_scale(wish_dir, 1.0f / wish_len);
    }

    // Apply acceleration towards wish direction
    float target_speed = player->is_crouching ? PLAYER_CROUCH_SPEED : PLAYER_WALK_SPEED;
    Vec3 target_vel = vec3_scale(wish_dir, target_speed);

    // Ground movement: accelerate towards target, apply friction
    float speed_diff_x = target_vel.x - player->velocity.x;
    float speed_diff_z = target_vel.z - player->velocity.z;

    float accel_rate = PLAYER_ACCEL;
    if (wish_len < 0.001f) {
        accel_rate = PLAYER_FRICTION * target_speed;
    }

    player->velocity.x += speed_diff_x * fminf(accel_rate * dt, 1.0f);
    player->velocity.z += speed_diff_z * fminf(accel_rate * dt, 1.0f);

    // Track if player is walking (for footstep bob)
    float horiz_speed = sqrtf(player->velocity.x * player->velocity.x +
                              player->velocity.z * player->velocity.z);
    player->is_walking = (horiz_speed > 0.5f && player->on_ground) ? 1 : 0;

    // ---- Jump ----
    if (input_key_pressed(input, SDL_SCANCODE_SPACE) && player->on_ground) {
        player->velocity.y = PLAYER_JUMP_SPEED;
        player->on_ground = 0;
    }

    // ---- Gravity ----
    if (!player->on_ground) {
        player->velocity.y -= PLAYER_GRAVITY * dt;
    }

    // Apply velocity
    player->position.x += player->velocity.x * dt;
    player->position.z += player->velocity.z * dt;
    player->foot_y += player->velocity.y * dt;

    // Ground collision (floor at y=0)
    if (player->foot_y <= 0.0f) {
        player->foot_y = 0.0f;
        player->velocity.y = 0.0f;
        player->on_ground = 1;
    }

    // ---- Eye height interpolation ----
    float target_eye = player->is_crouching ? PLAYER_CROUCH_EYE_HEIGHT : PLAYER_EYE_HEIGHT;
    float eye_diff = target_eye - player->current_eye_height;
    player->current_eye_height += eye_diff * fminf(PLAYER_EYE_SMOOTH * dt, 1.0f);

    // Set camera position: feet + eye height
    player->position.y = player->foot_y + player->current_eye_height;

    // ---- Gun bob animation ----
    if (player->is_walking) {
        player->gun_bob_timer += dt * horiz_speed * 1.2f;
    } else {
        // Settle back to rest
        player->gun_bob_timer *= (1.0f - dt * 5.0f);
    }

    // Gun recoil decay
    if (player->gun_recoil > 0.0f) {
        player->gun_recoil -= dt * 8.0f;
        if (player->gun_recoil < 0.0f) player->gun_recoil = 0.0f;
    }

    // ---- Weapon ----
    if (player->fire_cooldown > 0.0f) {
        player->fire_cooldown -= dt;
    }

    // Handle reload
    if (player->weapon_state == WEAPON_RELOADING) {
        player->reload_timer -= dt;
        if (player->reload_timer <= 0.0f) {
            player->ammo = player->max_ammo;
            player->weapon_state = WEAPON_READY;
        }
    }

    // Manual reload (R key)
    if (input_key_pressed(input, SDL_SCANCODE_R) &&
        player->weapon_state != WEAPON_RELOADING &&
        player->ammo < player->max_ammo) {
        player->weapon_state = WEAPON_RELOADING;
        player->reload_timer = WEAPON_RELOAD_TIME;
    }

    // Auto-reload on empty
    if (player->ammo <= 0 && player->weapon_state != WEAPON_RELOADING) {
        player->weapon_state = WEAPON_RELOADING;
        player->reload_timer = WEAPON_RELOAD_TIME;
    }

    player_update_bounds(player);
}

void player_update_bounds(Player* player) {
    float height = player->is_crouching ? PLAYER_CROUCH_HEIGHT : PLAYER_HEIGHT;
    player->bounds.min = vec3(
        player->position.x - PLAYER_RADIUS,
        player->foot_y,
        player->position.z - PLAYER_RADIUS
    );
    player->bounds.max = vec3(
        player->position.x + PLAYER_RADIUS,
        player->foot_y + height,
        player->position.z + PLAYER_RADIUS
    );
}

Mat4 player_get_view_matrix(Player* player) {
    float yaw_rad   = DEG2RAD(player->yaw);
    float pitch_rad = DEG2RAD(player->pitch);

    // Camera look direction
    Vec3 look_dir = vec3(
        -sinf(yaw_rad) * cosf(pitch_rad),
        -sinf(pitch_rad),
        -cosf(yaw_rad) * cosf(pitch_rad)
    );

    Vec3 eye = player->position;
    Vec3 center = vec3_add(eye, look_dir);
    Vec3 up = vec3(0, 1, 0);

    return mat4_look_at(eye, center, up);
}

Ray player_get_aim_ray(Player* player) {
    float yaw_rad   = DEG2RAD(player->yaw);
    float pitch_rad = DEG2RAD(player->pitch);

    Ray ray;
    ray.origin = player->position;
    ray.direction = vec3(
        -sinf(yaw_rad) * cosf(pitch_rad),
        -sinf(pitch_rad),
        -cosf(yaw_rad) * cosf(pitch_rad)
    );
    return ray;
}
