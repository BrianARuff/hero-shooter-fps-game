#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

#define MAX_KEYS 512

typedef struct {
    // Keyboard state
    int keys[MAX_KEYS];          // Current frame
    int keys_prev[MAX_KEYS];     // Previous frame

    // Mouse state
    float mouse_dx;              // Raw mouse delta X this frame
    float mouse_dy;              // Raw mouse delta Y this frame
    float sensitivity;           // Mouse sensitivity multiplier
    int mouse_buttons;           // Bitmask of current mouse buttons
    int mouse_buttons_prev;
    int invert_x;                // 1 = invert horizontal mouse
    int invert_y;                // 1 = invert vertical mouse

    // Window state
    int quit_requested;
    int window_w;
    int window_h;
    int window_resized;
} InputState;

void input_init(InputState* input, int window_w, int window_h);
void input_begin_frame(InputState* input);
void input_process_event(InputState* input, SDL_Event* event);

static inline int input_key_held(InputState* input, int scancode) {
    if (scancode < 0 || scancode >= MAX_KEYS) return 0;
    return input->keys[scancode];
}

static inline int input_key_pressed(InputState* input, int scancode) {
    if (scancode < 0 || scancode >= MAX_KEYS) return 0;
    return input->keys[scancode] && !input->keys_prev[scancode];
}

static inline int input_key_released(InputState* input, int scancode) {
    if (scancode < 0 || scancode >= MAX_KEYS) return 0;
    return !input->keys[scancode] && input->keys_prev[scancode];
}

static inline int input_mouse_held(InputState* input, int button) {
    return (input->mouse_buttons & SDL_BUTTON(button)) != 0;
}

static inline int input_mouse_pressed(InputState* input, int button) {
    return (input->mouse_buttons & SDL_BUTTON(button)) && !(input->mouse_buttons_prev & SDL_BUTTON(button));
}

#endif // INPUT_H
