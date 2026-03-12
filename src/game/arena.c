#include "arena.h"
#include "player.h"  // For PLAYER_HEIGHT constant
#include <math.h>

static void add_target(Arena* arena, float x, float z, float height) {
    if (arena->num_targets >= MAX_TARGETS) return;
    Target* t = &arena->targets[arena->num_targets++];
    t->size = vec3(0.6f, height, 0.3f);  // Humanoid-ish width
    t->position = vec3(x, height * 0.5f, z);
    t->health = 200;
    t->max_health = 200;
    t->alive = 1;
    t->respawn_timer = 0;
    t->hit_flash = 0;
}

static void add_wall(Arena* arena, Vec3 pos, Vec3 size) {
    if (arena->num_walls >= MAX_WALLS) return;
    Wall* w = &arena->walls[arena->num_walls++];
    w->position = pos;
    w->size = size;
    w->bounds.min = vec3(pos.x - size.x * 0.5f, pos.y - size.y * 0.5f, pos.z - size.z * 0.5f);
    w->bounds.max = vec3(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, pos.z + size.z * 0.5f);
}

void arena_init(Arena* arena) {
    arena->floor_size = 80.0f;
    arena->floor_y = 0.0f;
    arena->num_targets = 0;
    arena->num_walls = 0;

    // Arena bounds
    float half = arena->floor_size * 0.5f;
    arena->bounds.min = vec3(-half, 0, -half);
    arena->bounds.max = vec3(half, 20, half);

    // ---- Targets at varying distances ----
    // Close range: 5m
    add_target(arena, 0, -5.0f, 1.8f);
    add_target(arena, 2, -5.0f, 1.8f);

    // Medium range: 10m
    add_target(arena, -3, -10.0f, 1.8f);
    add_target(arena, 0, -10.0f, 1.8f);
    add_target(arena, 3, -10.0f, 1.8f);

    // Long range: 20m
    add_target(arena, -5, -20.0f, 1.8f);
    add_target(arena, 0, -20.0f, 1.8f);
    add_target(arena, 5, -20.0f, 1.8f);

    // Far range: 40m
    add_target(arena, -4, -40.0f, 1.8f);
    add_target(arena, 0, -40.0f, 1.8f);
    add_target(arena, 4, -40.0f, 1.8f);

    // ---- Perimeter walls ----
    float wall_h = 4.0f;
    float wall_thick = 0.5f;

    // North wall
    add_wall(arena, vec3(0, wall_h * 0.5f, -half), vec3(arena->floor_size, wall_h, wall_thick));
    // South wall
    add_wall(arena, vec3(0, wall_h * 0.5f, half), vec3(arena->floor_size, wall_h, wall_thick));
    // East wall
    add_wall(arena, vec3(half, wall_h * 0.5f, 0), vec3(wall_thick, wall_h, arena->floor_size));
    // West wall
    add_wall(arena, vec3(-half, wall_h * 0.5f, 0), vec3(wall_thick, wall_h, arena->floor_size));

    // ---- Ramp for height testing ----
    // A simple raised platform with a ramp-like step
    add_wall(arena, vec3(15, 0.5f, -15), vec3(6, 1.0f, 6));   // Low platform
    add_wall(arena, vec3(15, 1.5f, -20), vec3(6, 3.0f, 4));   // Back wall of platform

    // A couple of cover walls inside the arena
    add_wall(arena, vec3(-8, 1.5f, -15), vec3(3, 3.0f, 0.4f));
    add_wall(arena, vec3(8, 1.5f, -8),   vec3(0.4f, 3.0f, 3));
}

void arena_update(Arena* arena, float dt) {
    for (int i = 0; i < arena->num_targets; i++) {
        Target* t = &arena->targets[i];

        // Hit flash decay
        if (t->hit_flash > 0) {
            t->hit_flash -= dt * 4.0f;
            if (t->hit_flash < 0) t->hit_flash = 0;
        }

        // Respawn dead targets
        if (!t->alive) {
            t->respawn_timer -= dt;
            if (t->respawn_timer <= 0) {
                t->alive = 1;
                t->health = t->max_health;
            }
        }
    }
}

void arena_render(Arena* arena, RenderState* render_state) {
    // Draw floor
    renderer_draw_floor(render_state, arena->floor_size, arena->floor_y);

    // Draw targets
    for (int i = 0; i < arena->num_targets; i++) {
        Target* t = &arena->targets[i];
        if (!t->alive) continue;

        float r, g, b;
        if (t->hit_flash > 0) {
            // Flash red when hit
            r = 1.0f;
            g = 0.2f * (1.0f - t->hit_flash);
            b = 0.2f * (1.0f - t->hit_flash);
        } else {
            // Health-based color: green -> yellow -> red
            float health_frac = (float)t->health / t->max_health;
            if (health_frac > 0.5f) {
                r = 1.0f - (health_frac - 0.5f) * 2.0f;
                g = 0.8f;
                b = 0.2f;
            } else {
                r = 1.0f;
                g = health_frac * 1.6f;
                b = 0.2f;
            }
        }

        renderer_draw_box(render_state, t->position, t->size, r, g, b, 1.0f);

        // Draw health bar above target
        float bar_y = t->position.y + t->size.y * 0.5f + 0.3f;
        float bar_width = t->size.x;
        float health_frac = (float)t->health / t->max_health;

        // Background
        renderer_draw_box(render_state,
            vec3(t->position.x, bar_y, t->position.z),
            vec3(bar_width, 0.08f, 0.08f),
            0.2f, 0.2f, 0.2f, 0.8f);

        // Health fill
        if (health_frac > 0.01f) {
            float fill_width = bar_width * health_frac;
            float offset = (bar_width - fill_width) * 0.5f;
            renderer_draw_box(render_state,
                vec3(t->position.x - offset, bar_y, t->position.z),
                vec3(fill_width, 0.06f, 0.06f),
                0.2f, 1.0f, 0.2f, 0.9f);
        }
    }

    // Draw walls
    for (int i = 0; i < arena->num_walls; i++) {
        Wall* w = &arena->walls[i];
        renderer_draw_box(render_state, w->position, w->size, 0.4f, 0.4f, 0.45f, 0.9f);
    }

    // Draw distance markers on the floor
    for (int dist = 5; dist <= 40; dist += 5) {
        float z = -(float)dist;
        renderer_draw_line(render_state,
            vec3(-2, 0.01f, z), vec3(2, 0.01f, z),
            0.5f, 0.5f, 0.2f);
    }
}

int arena_raycast_targets(Arena* arena, Ray ray, float max_dist, float* out_distance) {
    int best_idx = -1;
    float best_t = max_dist;

    for (int i = 0; i < arena->num_targets; i++) {
        Target* t = &arena->targets[i];
        if (!t->alive) continue;

        AABB target_box;
        target_box.min = vec3(
            t->position.x - t->size.x * 0.5f,
            t->position.y - t->size.y * 0.5f,
            t->position.z - t->size.z * 0.5f
        );
        target_box.max = vec3(
            t->position.x + t->size.x * 0.5f,
            t->position.y + t->size.y * 0.5f,
            t->position.z + t->size.z * 0.5f
        );

        float hit_t = ray_aabb_intersect(ray, target_box);
        if (hit_t >= 0 && hit_t < best_t) {
            best_t = hit_t;
            best_idx = i;
        }
    }

    if (out_distance) *out_distance = best_t;
    return best_idx;
}

int arena_check_wall_collision(Arena* arena, Vec3 pos, float radius) {
    AABB player_box;
    player_box.min = vec3(pos.x - radius, 0, pos.z - radius);
    player_box.max = vec3(pos.x + radius, PLAYER_HEIGHT, pos.z + radius);

    for (int i = 0; i < arena->num_walls; i++) {
        if (aabb_intersect(player_box, arena->walls[i].bounds)) {
            return 1;
        }
    }

    // Check arena bounds
    float half = arena->floor_size * 0.5f;
    if (pos.x - radius < -half || pos.x + radius > half ||
        pos.z - radius < -half || pos.z + radius > half) {
        return 1;
    }

    return 0;
}

Vec3 arena_resolve_collision(Arena* arena, Vec3 pos, float radius) {
    float half = arena->floor_size * 0.5f;

    // Clamp to arena bounds
    pos.x = clampf(pos.x, -half + radius, half - radius);
    pos.z = clampf(pos.z, -half + radius, half - radius);

    // Push out of walls (simple AABB resolution)
    for (int i = 0; i < arena->num_walls; i++) {
        Wall* w = &arena->walls[i];
        AABB wb = w->bounds;

        // Check if player circle overlaps wall AABB (on XZ plane)
        float closest_x = clampf(pos.x, wb.min.x, wb.max.x);
        float closest_z = clampf(pos.z, wb.min.z, wb.max.z);

        float dx = pos.x - closest_x;
        float dz = pos.z - closest_z;
        float dist_sq = dx * dx + dz * dz;

        if (dist_sq < radius * radius && dist_sq > 0.0001f) {
            float dist = sqrtf(dist_sq);
            float push = radius - dist;
            pos.x += (dx / dist) * push;
            pos.z += (dz / dist) * push;
        } else if (dist_sq < 0.0001f) {
            // Player center is inside wall, push out along smallest axis
            float push_left  = pos.x - wb.min.x + radius;
            float push_right = wb.max.x - pos.x + radius;
            float push_front = pos.z - wb.min.z + radius;
            float push_back  = wb.max.z - pos.z + radius;

            float min_push = push_left;
            int axis = 0; // 0=left, 1=right, 2=front, 3=back

            if (push_right < min_push) { min_push = push_right; axis = 1; }
            if (push_front < min_push) { min_push = push_front; axis = 2; }
            if (push_back  < min_push) { min_push = push_back;  axis = 3; }

            switch (axis) {
                case 0: pos.x = wb.min.x - radius; break;
                case 1: pos.x = wb.max.x + radius; break;
                case 2: pos.z = wb.min.z - radius; break;
                case 3: pos.z = wb.max.z + radius; break;
            }
        }
    }

    return pos;
}
