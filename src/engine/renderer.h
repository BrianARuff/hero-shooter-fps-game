#ifndef RENDERER_H
#define RENDERER_H

#include "../util/math_utils.h"

// Immediate-mode style 3D renderer using OpenGL 2.1 (compatible everywhere)
// Will be replaced with modern GL/Vulkan renderer later

typedef struct {
    float fov;
    float aspect;
    float near_plane;
    float far_plane;
    Mat4 projection;
    Mat4 view;
} RenderState;

void renderer_init(RenderState* state, float fov, float aspect);
void renderer_set_fov(RenderState* state, float fov);
void renderer_begin_frame(RenderState* state);
void renderer_set_view(RenderState* state, Mat4 view);
void renderer_draw_box(RenderState* state, Vec3 pos, Vec3 size, float r, float g, float b, float a);
void renderer_draw_floor(RenderState* state, float size, float y);
void renderer_draw_line(RenderState* state, Vec3 from, Vec3 to, float r, float g, float b);
void renderer_draw_crosshair(int screen_w, int screen_h);
void renderer_draw_hud(int screen_w, int screen_h, int health, int max_health, int ammo, int max_ammo);
void renderer_draw_text_simple(float x, float y, const char* text, float r, float g, float b);

#endif // RENDERER_H
