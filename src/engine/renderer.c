#include "renderer.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void renderer_init(RenderState* state, float fov, float aspect) {
    state->fov = fov;
    state->aspect = aspect;
    state->near_plane = 0.1f;
    state->far_plane = 1000.0f;
    state->projection = mat4_perspective(fov, aspect, 0.1f, 1000.0f);
    state->view = mat4_identity();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
}

void renderer_set_fov(RenderState* state, float fov) {
    state->fov = fov;
    state->projection = mat4_perspective(fov, state->aspect, state->near_plane, state->far_plane);
}

void renderer_begin_frame(RenderState* state) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(state->projection.m);

    // Set view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(state->view.m);
}

void renderer_set_view(RenderState* state, Mat4 view) {
    state->view = view;
}

void renderer_draw_box(RenderState* state, Vec3 pos, Vec3 size, float r, float g, float b, float a) {
    glPushMatrix();

    Mat4 model = mat4_translate(pos);
    Mat4 mv = mat4_multiply(state->view, model);
    glLoadMatrixf(mv.m);

    float hx = size.x * 0.5f;
    float hy = size.y * 0.5f;
    float hz = size.z * 0.5f;

    glBegin(GL_QUADS);
    glColor4f(r, g, b, a);

    // Front
    glVertex3f(-hx, -hy,  hz);
    glVertex3f( hx, -hy,  hz);
    glVertex3f( hx,  hy,  hz);
    glVertex3f(-hx,  hy,  hz);

    // Back
    glVertex3f( hx, -hy, -hz);
    glVertex3f(-hx, -hy, -hz);
    glVertex3f(-hx,  hy, -hz);
    glVertex3f( hx,  hy, -hz);

    // Left
    glVertex3f(-hx, -hy, -hz);
    glVertex3f(-hx, -hy,  hz);
    glVertex3f(-hx,  hy,  hz);
    glVertex3f(-hx,  hy, -hz);

    // Right
    glVertex3f( hx, -hy,  hz);
    glVertex3f( hx, -hy, -hz);
    glVertex3f( hx,  hy, -hz);
    glVertex3f( hx,  hy,  hz);

    // Top
    glVertex3f(-hx,  hy,  hz);
    glVertex3f( hx,  hy,  hz);
    glVertex3f( hx,  hy, -hz);
    glVertex3f(-hx,  hy, -hz);

    // Bottom
    glVertex3f(-hx, -hy, -hz);
    glVertex3f( hx, -hy, -hz);
    glVertex3f( hx, -hy,  hz);
    glVertex3f(-hx, -hy,  hz);

    glEnd();

    // Draw wireframe edges for visibility
    glColor4f(r * 0.5f, g * 0.5f, b * 0.5f, a);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz);
    glVertex3f(hx, hy, hz);   glVertex3f(-hx, hy, hz);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(-hx, -hy, -hz); glVertex3f(hx, -hy, -hz);
    glVertex3f(hx, hy, -hz);   glVertex3f(-hx, hy, -hz);
    glEnd();

    glPopMatrix();
}

void renderer_draw_floor(RenderState* state, float size, float y) {
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(state->view.m);

    float half = size * 0.5f;
    float grid_size = 2.0f;

    // Main floor quad
    glBegin(GL_QUADS);
    glColor4f(0.25f, 0.25f, 0.3f, 1.0f);
    glVertex3f(-half, y, -half);
    glVertex3f( half, y, -half);
    glVertex3f( half, y,  half);
    glVertex3f(-half, y,  half);
    glEnd();

    // Grid lines
    glBegin(GL_LINES);
    glColor4f(0.35f, 0.35f, 0.4f, 0.5f);
    for (float i = -half; i <= half; i += grid_size) {
        glVertex3f(i, y + 0.005f, -half);
        glVertex3f(i, y + 0.005f, half);
        glVertex3f(-half, y + 0.005f, i);
        glVertex3f(half, y + 0.005f, i);
    }
    glEnd();
}

void renderer_draw_line(RenderState* state, Vec3 from, Vec3 to, float r, float g, float b) {
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(state->view.m);

    glBegin(GL_LINES);
    glColor3f(r, g, b);
    glVertex3f(from.x, from.y, from.z);
    glVertex3f(to.x, to.y, to.z);
    glEnd();
}

void renderer_draw_crosshair(int screen_w, int screen_h) {
    // Switch to 2D ortho projection
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
    float gap = 4.0f;
    float length = 8.0f;
    float thickness = 1.5f;

    // Crosshair lines (white with black outline for visibility)
    // Outline
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    // Top
    glVertex2f(cx, cy - gap - length);
    glVertex2f(cx, cy - gap);
    // Bottom
    glVertex2f(cx, cy + gap);
    glVertex2f(cx, cy + gap + length);
    // Left
    glVertex2f(cx - gap - length, cy);
    glVertex2f(cx - gap, cy);
    // Right
    glVertex2f(cx + gap, cy);
    glVertex2f(cx + gap + length, cy);
    glEnd();

    // White inner lines
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(cx, cy - gap - length);
    glVertex2f(cx, cy - gap);
    glVertex2f(cx, cy + gap);
    glVertex2f(cx, cy + gap + length);
    glVertex2f(cx - gap - length, cy);
    glVertex2f(cx - gap, cy);
    glVertex2f(cx + gap, cy);
    glVertex2f(cx + gap + length, cy);
    glEnd();

    // Center dot
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex2f(cx, cy);
    glEnd();

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// Simple bitmap font rendering using GL lines (no texture dependencies)
static void draw_char(float x, float y, char c, float scale) {
    // Minimal 5x7 bitmap-style characters drawn with lines
    // Only digits, letters, and common symbols for HUD
    float s = scale;

    #define L(x1,y1,x2,y2) glVertex2f(x + (x1)*s, y + (y1)*s); glVertex2f(x + (x2)*s, y + (y2)*s);

    glBegin(GL_LINES);

    if (c >= '0' && c <= '9') {
        switch(c) {
            case '0': L(0,0,4,0) L(4,0,4,6) L(4,6,0,6) L(0,6,0,0) break;
            case '1': L(2,0,2,6) L(0,6,4,6) L(0,0,2,0) break;
            case '2': L(0,0,4,0) L(4,0,4,3) L(4,3,0,3) L(0,3,0,6) L(0,6,4,6) break;
            case '3': L(0,0,4,0) L(4,0,4,6) L(4,6,0,6) L(0,3,4,3) break;
            case '4': L(0,0,0,3) L(0,3,4,3) L(4,0,4,6) break;
            case '5': L(4,0,0,0) L(0,0,0,3) L(0,3,4,3) L(4,3,4,6) L(4,6,0,6) break;
            case '6': L(4,0,0,0) L(0,0,0,6) L(0,6,4,6) L(4,6,4,3) L(4,3,0,3) break;
            case '7': L(0,0,4,0) L(4,0,4,6) break;
            case '8': L(0,0,4,0) L(4,0,4,6) L(4,6,0,6) L(0,6,0,0) L(0,3,4,3) break;
            case '9': L(0,0,4,0) L(4,0,4,6) L(4,6,0,6) L(0,0,0,3) L(0,3,4,3) break;
        }
    } else if (c >= 'A' && c <= 'Z') {
        switch(c) {
            case 'A': L(0,6,0,0) L(0,0,4,0) L(4,0,4,6) L(0,3,4,3) break;
            case 'B': L(0,0,3,0) L(3,0,4,1) L(4,1,3,3) L(0,3,3,3) L(3,3,4,4) L(4,4,4,5) L(4,5,3,6) L(3,6,0,6) L(0,0,0,6) break;
            case 'C': L(4,0,0,0) L(0,0,0,6) L(0,6,4,6) break;
            case 'D': L(0,0,3,0) L(3,0,4,2) L(4,2,4,4) L(4,4,3,6) L(3,6,0,6) L(0,0,0,6) break;
            case 'E': L(4,0,0,0) L(0,0,0,6) L(0,6,4,6) L(0,3,3,3) break;
            case 'F': L(4,0,0,0) L(0,0,0,6) L(0,3,3,3) break;
            case 'H': L(0,0,0,6) L(4,0,4,6) L(0,3,4,3) break;
            case 'I': L(1,0,3,0) L(2,0,2,6) L(1,6,3,6) break;
            case 'K': L(0,0,0,6) L(4,0,0,3) L(0,3,4,6) break;
            case 'L': L(0,0,0,6) L(0,6,4,6) break;
            case 'M': L(0,6,0,0) L(0,0,2,3) L(2,3,4,0) L(4,0,4,6) break;
            case 'N': L(0,6,0,0) L(0,0,4,6) L(4,6,4,0) break;
            case 'O': L(0,0,4,0) L(4,0,4,6) L(4,6,0,6) L(0,6,0,0) break;
            case 'P': L(0,0,4,0) L(4,0,4,3) L(4,3,0,3) L(0,0,0,6) break;
            case 'R': L(0,0,4,0) L(4,0,4,3) L(4,3,0,3) L(0,0,0,6) L(2,3,4,6) break;
            case 'S': L(4,0,0,0) L(0,0,0,3) L(0,3,4,3) L(4,3,4,6) L(4,6,0,6) break;
            case 'T': L(0,0,4,0) L(2,0,2,6) break;
            case 'V': L(0,0,2,6) L(2,6,4,0) break;
            case 'W': L(0,0,1,6) L(1,6,2,3) L(2,3,3,6) L(3,6,4,0) break;
            case 'X': L(0,0,4,6) L(4,0,0,6) break;
            default: L(0,0,4,0) L(4,0,4,6) L(4,6,0,6) L(0,6,0,0) break; // Box for unknown
        }
    } else if (c >= 'a' && c <= 'z') {
        // Render lowercase as uppercase
        draw_char(x, y, c - 32, scale);
        glBegin(GL_LINES); // Re-open since recursive call closed it
    } else if (c == '/') {
        L(0,6,4,0)
    } else if (c == ':') {
        L(2,1,2,2) L(2,4,2,5)
    } else if (c == '.') {
        L(2,5,2,6)
    } else if (c == '-') {
        L(1,3,3,3)
    } else if (c == '+') {
        L(0,3,4,3) L(2,1,2,5)
    } else if (c == '%') {
        L(0,0,4,6) L(0,0,1,1) L(3,5,4,6)
    }

    #undef L

    glEnd();
}

void renderer_draw_text_simple(float x, float y, const char* text, float r, float g, float b) {
    glColor3f(r, g, b);
    glLineWidth(1.5f);

    float cursor_x = x;
    for (int i = 0; text[i]; i++) {
        if (text[i] == ' ') {
            cursor_x += 8.0f;
            continue;
        }
        draw_char(cursor_x, y, text[i], 2.0f);
        cursor_x += 10.0f;
    }
}

void renderer_draw_hud(int screen_w, int screen_h, int health, int ammo, int max_ammo) {
    // Switch to 2D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screen_w, screen_h, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    // Health bar background
    float hbar_x = 20.0f;
    float hbar_y = screen_h - 40.0f;
    float hbar_w = 200.0f;
    float hbar_h = 20.0f;

    glBegin(GL_QUADS);
    glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
    glVertex2f(hbar_x, hbar_y);
    glVertex2f(hbar_x + hbar_w, hbar_y);
    glVertex2f(hbar_x + hbar_w, hbar_y + hbar_h);
    glVertex2f(hbar_x, hbar_y + hbar_h);

    // Health bar fill
    float health_frac = health / 200.0f; // Max health 200
    float fill_w = hbar_w * health_frac;
    float hr = health > 100 ? 0.2f : (health > 50 ? 1.0f : 1.0f);
    float hg = health > 100 ? 0.8f : (health > 50 ? 0.8f : 0.2f);
    float hb = health > 100 ? 0.2f : 0.2f;
    glColor4f(hr, hg, hb, 0.9f);
    glVertex2f(hbar_x, hbar_y);
    glVertex2f(hbar_x + fill_w, hbar_y);
    glVertex2f(hbar_x + fill_w, hbar_y + hbar_h);
    glVertex2f(hbar_x, hbar_y + hbar_h);
    glEnd();

    // Health text
    char health_text[32];
    snprintf(health_text, sizeof(health_text), "%d", health);
    renderer_draw_text_simple(hbar_x + hbar_w + 10.0f, hbar_y + 2.0f, health_text, 1.0f, 1.0f, 1.0f);

    // Ammo display
    char ammo_text[32];
    snprintf(ammo_text, sizeof(ammo_text), "%d / %d", ammo, max_ammo);
    renderer_draw_text_simple(screen_w - 120.0f, screen_h - 38.0f, ammo_text, 1.0f, 1.0f, 1.0f);

    // FPS display area (will be filled by main loop)

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
