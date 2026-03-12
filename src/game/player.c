#include "player.h"
#include <math.h>

void player_init(Player* player, Vec3 spawn_pos) {
    player->position = spawn_pos;
    player->velocity = vec3(0, 0, 0);
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    player->health = PLAYER_MAX_HEALTH;
    player->alive = 1;

    player->weapon_state = WEAPON_READY;
    player->ammo = WEAPON_MAG_SIZE;
    player->max_ammo = WEAPON_MAG_SIZE;
    player->fire_cooldown = 0.0f;
    player->reload_timer = 0.0f;

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

    // ---- Mouse look ----
    // Movement does NOT affect aim (per spec), so we always apply mouse input
    float sens = input->sensitivity;
    player->yaw   += input->mouse_dx * sens;
    player->pitch  += input->mouse_dy * sens;

    // Clamp pitch to prevent flipping
    if (player->pitch > 89.0f)  player->pitch = 89.0f;
    if (player->pitch < -89.0f) player->pitch = -89.0f;

    // Normalize yaw to [0, 360)
    while (player->yaw >= 360.0f) player->yaw -= 360.0f;
    while (player->yaw < 0.0f)    player->yaw += 360.0f;

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
    float target_speed = PLAYER_WALK_SPEED;
    Vec3 target_vel = vec3_scale(wish_dir, target_speed);

    // Ground movement: accelerate towards target, apply friction
    float speed_diff_x = target_vel.x - player->velocity.x;
    float speed_diff_z = target_vel.z - player->velocity.z;

    float accel_rate = PLAYER_ACCEL;
    // Apply friction when no input or when changing direction
    if (wish_len < 0.001f) {
        accel_rate = PLAYER_FRICTION * target_speed;
    }

    player->velocity.x += speed_diff_x * fminf(accel_rate * dt, 1.0f);
    player->velocity.z += speed_diff_z * fminf(accel_rate * dt, 1.0f);

    // Apply velocity
    player->position.x += player->velocity.x * dt;
    player->position.z += player->velocity.z * dt;

    // Keep player on the ground (no jumping in V1)
    player->position.y = PLAYER_EYE_HEIGHT;
    player->velocity.y = 0.0f;

    // ---- Weapon ----
    // Decrease fire cooldown
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

    // Firing handled in main game loop (needs target info)

    player_update_bounds(player);
}

void player_update_bounds(Player* player) {
    player->bounds.min = vec3(
        player->position.x - PLAYER_RADIUS,
        player->position.y - PLAYER_EYE_HEIGHT,
        player->position.z - PLAYER_RADIUS
    );
    player->bounds.max = vec3(
        player->position.x + PLAYER_RADIUS,
        player->position.y + (PLAYER_HEIGHT - PLAYER_EYE_HEIGHT),
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
