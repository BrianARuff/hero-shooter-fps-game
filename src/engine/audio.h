#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>

// Maximum simultaneous sounds
#define AUDIO_MAX_VOICES 16
#define AUDIO_SAMPLE_RATE 44100

typedef enum {
    SOUND_NONE = 0,
    SOUND_FOOTSTEP,
    SOUND_JUMP,
    SOUND_LAND,
    SOUND_GUNSHOT,
    SOUND_RELOAD
} SoundType;

typedef struct {
    SoundType type;
    float phase;          // Current playback position (in samples)
    float duration;       // Total duration in seconds
    float volume;         // 0.0 - 1.0
    float frequency;      // Base frequency for tonal sounds
    int active;           // 1 = playing
} AudioVoice;

typedef struct {
    SDL_AudioDeviceID device;
    AudioVoice voices[AUDIO_MAX_VOICES];
    int initialized;
    float master_volume;

    // Footstep timing
    float footstep_timer;
    float footstep_interval;  // Time between footsteps
    int footstep_alt;         // Alternates left/right foot
} AudioState;

void audio_init(AudioState* audio);
void audio_shutdown(AudioState* audio);
void audio_play_sound(AudioState* audio, SoundType type);
void audio_update_footsteps(AudioState* audio, int is_walking, int is_crouching, float dt);

#endif // AUDIO_H
