#ifndef PLAYER_H
#define PLAYER_H

#include "../util/math_utils.h"
#include "../engine/input.h"

// Player constants (Soldier: 76 style)
#define PLAYER_WALK_SPEED     5.5f    // m/s - OW Soldier: 76 base speed
#define PLAYER_HEIGHT         1.8f    // meters
#define PLAYER_EYE_HEIGHT     1.6f    // meters (camera position)
#define PLAYER_CROUCH_EYE_HEIGHT 1.0f // meters (crouched camera position)
#define PLAYER_CROUCH_HEIGHT  1.2f    // meters (crouched collision height)
#define PLAYER_CROUCH_SPEED   2.75f   // m/s (half walk speed)
#define PLAYER_RADIUS         0.3f    // collision radius
#define PLAYER_MAX_HEALTH     200     // Soldier: 76 has 200 HP
#define PLAYER_ACCEL          50.0f   // Ground acceleration (snappy feel)
#define PLAYER_FRICTION       10.0f   // Ground friction
#define PLAYER_GRAVITY        20.0f   // m/s^2 (slightly higher than real for game feel)
#define PLAYER_JUMP_SPEED     7.5f    // m/s initial upward velocity
#define PLAYER_EYE_SMOOTH     15.0f   // Eye height interpolation speed

// Weapon constants
#define WEAPON_DAMAGE         19      // Per bullet (OW Heavy Pulse Rifle)
#define WEAPON_FIRE_RATE      9.0f    // Rounds per second
#define WEAPON_MAG_SIZE       25      // Magazine size
#define WEAPON_RELOAD_TIME    1.5f    // Seconds
#define WEAPON_RANGE          100.0f  // Max hitscan range in meters

typedef enum {
    WEAPON_READY,
    WEAPON_FIRING,
    WEAPON_RELOADING
} WeaponState;

typedef struct {
    // Position & physics
    Vec3 position;          // position.y = feet position (ground level when standing)
    Vec3 velocity;
    float yaw;              // Horizontal look angle (degrees)
    float pitch;            // Vertical look angle (degrees)

    // Movement state
    int on_ground;          // 1 if player is on the ground
    int is_crouching;       // 1 if crouch key is held
    float current_eye_height; // Smoothly interpolated eye height
    float foot_y;           // Y position of player's feet

    // Player state
    int health;
    int alive;

    // Weapon state
    WeaponState weapon_state;
    int ammo;
    int max_ammo;
    float fire_cooldown;    // Time until next shot
    float reload_timer;     // Time remaining for reload

    // Gun viewmodel animation
    float gun_recoil;       // Current recoil offset (0 = resting)
    float gun_bob_timer;    // Walking bob animation timer
    int is_walking;         // 1 if player is moving (for footstep bob)

    // Collision
    AABB bounds;            // Updated each physics tick

    // Stats
    int shots_fired;
    int shots_hit;
} Player;

void player_init(Player* player, Vec3 spawn_pos);
void player_update(Player* player, InputState* input, float dt);
Mat4 player_get_view_matrix(Player* player);
Ray  player_get_aim_ray(Player* player);
void player_update_bounds(Player* player);

// Get forward/right vectors from current look angles
Vec3 player_get_forward(Player* player);
Vec3 player_get_right(Player* player);

#endif // PLAYER_H
