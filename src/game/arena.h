#ifndef ARENA_H
#define ARENA_H

#include "../util/math_utils.h"
#include "../engine/renderer.h"

#define MAX_TARGETS 32
#define MAX_WALLS   64

typedef struct {
    Vec3 position;
    Vec3 size;
    int  health;
    int  max_health;
    int  alive;
    float respawn_timer;
    float hit_flash;        // Visual feedback timer
} Target;

typedef struct {
    Vec3 position;
    Vec3 size;
    AABB bounds;
} Wall;

typedef struct {
    // Floor
    float floor_size;
    float floor_y;

    // Targets
    Target targets[MAX_TARGETS];
    int    num_targets;

    // Walls (collision)
    Wall   walls[MAX_WALLS];
    int    num_walls;

    // Arena bounds
    AABB   bounds;
} Arena;

void arena_init(Arena* arena);
void arena_update(Arena* arena, float dt);
void arena_render(Arena* arena, RenderState* render_state);

// Returns index of hit target, -1 if none
int arena_raycast_targets(Arena* arena, Ray ray, float max_dist, float* out_distance);

// Check if a position would collide with walls
int arena_check_wall_collision(Arena* arena, Vec3 pos, float radius);

// Resolve player collision with walls, returns corrected position
Vec3 arena_resolve_collision(Arena* arena, Vec3 pos, float radius);

#endif // ARENA_H
