#include "input.h"
#include <string.h>

void input_init(InputState* input, int window_w, int window_h) {
    memset(input, 0, sizeof(InputState));
    input->sensitivity = 0.15f; // Reasonable default, similar to OW's "5" setting
    input->window_w = window_w;
    input->window_h = window_h;

    // Enable raw mouse input (bypasses OS acceleration)
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0");
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void input_begin_frame(InputState* input) {
    memcpy(input->keys_prev, input->keys, sizeof(input->keys));
    input->mouse_buttons_prev = input->mouse_buttons;
    input->mouse_dx = 0;
    input->mouse_dy = 0;
    input->window_resized = 0;
}

void input_process_event(InputState* input, SDL_Event* event) {
    switch (event->type) {
        case SDL_QUIT:
            input->quit_requested = 1;
            break;

        case SDL_KEYDOWN:
            if (event->key.keysym.scancode < MAX_KEYS) {
                input->keys[event->key.keysym.scancode] = 1;
            }
            // ESC to quit
            if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                input->quit_requested = 1;
            }
            break;

        case SDL_KEYUP:
            if (event->key.keysym.scancode < MAX_KEYS) {
                input->keys[event->key.keysym.scancode] = 0;
            }
            break;

        case SDL_MOUSEMOTION:
            // Raw relative mouse motion - no acceleration
            input->mouse_dx += (float)event->motion.xrel;
            input->mouse_dy += (float)event->motion.yrel;
            break;

        case SDL_MOUSEBUTTONDOWN:
            input->mouse_buttons |= SDL_BUTTON(event->button.button);
            break;

        case SDL_MOUSEBUTTONUP:
            input->mouse_buttons &= ~SDL_BUTTON(event->button.button);
            break;

        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                input->window_w = event->window.data1;
                input->window_h = event->window.data2;
                input->window_resized = 1;
            }
            break;
    }
}
