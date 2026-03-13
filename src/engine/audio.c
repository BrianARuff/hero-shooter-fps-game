#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Simple pseudo-random for noise generation (no malloc)
static unsigned int audio_rng_state = 12345;
static float audio_noise(void) {
    audio_rng_state = audio_rng_state * 1103515245 + 12345;
    return ((float)(audio_rng_state >> 16 & 0x7FFF) / 32767.0f) * 2.0f - 1.0f;
}

// Generate a sample for a given voice
static float generate_sample(AudioVoice* voice, float t) {
    float sample = 0.0f;
    float env = 1.0f;

    switch (voice->type) {
        case SOUND_FOOTSTEP: {
            // Short thud: low frequency noise burst with fast decay
            float progress = t / voice->duration;
            env = (1.0f - progress) * (1.0f - progress);  // Quick decay
            // Low-pass filtered noise (simulate thud)
            float noise = audio_noise();
            float freq = 80.0f + voice->frequency * 20.0f;
            float tone = sinf(2.0f * 3.14159f * freq * t) * 0.3f;
            sample = (noise * 0.4f + tone) * env;
            break;
        }
        case SOUND_JUMP: {
            // Rising pitch sweep
            float progress = t / voice->duration;
            env = 1.0f - progress;
            float freq = 200.0f + progress * 400.0f;  // Sweep up
            sample = sinf(2.0f * 3.14159f * freq * t) * 0.3f * env;
            sample += audio_noise() * 0.05f * env;  // Slight noise
            break;
        }
        case SOUND_LAND: {
            // Impact thud: heavy low thump
            float progress = t / voice->duration;
            env = (1.0f - progress) * (1.0f - progress) * (1.0f - progress);
            float freq = 60.0f;
            sample = sinf(2.0f * 3.14159f * freq * t) * 0.5f * env;
            sample += audio_noise() * 0.3f * env;
            break;
        }
        case SOUND_GUNSHOT: {
            // Sharp crack: white noise burst + low thump
            float progress = t / voice->duration;
            // Initial crack (first 10ms)
            float crack_env = (t < 0.01f) ? 1.0f : expf(-(t - 0.01f) * 40.0f);
            float thump_env = expf(-t * 15.0f);
            sample = audio_noise() * 0.6f * crack_env;
            sample += sinf(2.0f * 3.14159f * 120.0f * t) * 0.3f * thump_env;
            sample *= (1.0f - progress * 0.5f);
            break;
        }
        case SOUND_RELOAD: {
            // Mechanical click-clack sequence
            float progress = t / voice->duration;
            // Two click events at 0.2s and 0.8s into the sound
            float click1 = expf(-fabsf(t - 0.15f) * 80.0f);
            float click2 = expf(-fabsf(t - 0.5f) * 80.0f);
            sample = audio_noise() * 0.3f * (click1 + click2);
            sample += sinf(2.0f * 3.14159f * 800.0f * t) * 0.1f * click1;
            sample += sinf(2.0f * 3.14159f * 600.0f * t) * 0.1f * click2;
            sample *= (1.0f - progress * 0.3f);
            break;
        }
        default:
            break;
    }

    return sample * voice->volume;
}

// SDL audio callback
static void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioState* audio = (AudioState*)userdata;
    float* out = (float*)stream;
    int samples = len / sizeof(float);

    memset(stream, 0, len);

    for (int v = 0; v < AUDIO_MAX_VOICES; v++) {
        AudioVoice* voice = &audio->voices[v];
        if (!voice->active) continue;

        for (int i = 0; i < samples; i++) {
            float t = voice->phase / (float)AUDIO_SAMPLE_RATE;
            if (t >= voice->duration) {
                voice->active = 0;
                break;
            }
            out[i] += generate_sample(voice, t) * audio->master_volume;
            voice->phase += 1.0f;
        }
    }

    // Clamp output
    for (int i = 0; i < samples; i++) {
        if (out[i] > 1.0f) out[i] = 1.0f;
        if (out[i] < -1.0f) out[i] = -1.0f;
    }
}

void audio_init(AudioState* audio) {
    memset(audio, 0, sizeof(AudioState));
    audio->master_volume = 0.5f;
    audio->footstep_interval = 0.4f;

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = AUDIO_SAMPLE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    want.userdata = audio;

    audio->device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio->device == 0) {
        fprintf(stderr, "Audio init failed: %s\n", SDL_GetError());
        audio->initialized = 0;
        return;
    }

    audio->initialized = 1;
    SDL_PauseAudioDevice(audio->device, 0);  // Start playback
}

void audio_shutdown(AudioState* audio) {
    if (audio->initialized && audio->device) {
        SDL_CloseAudioDevice(audio->device);
    }
    audio->initialized = 0;
}

void audio_play_sound(AudioState* audio, SoundType type) {
    if (!audio->initialized) return;

    // Find a free voice
    SDL_LockAudioDevice(audio->device);

    int slot = -1;
    for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
        if (!audio->voices[i].active) {
            slot = i;
            break;
        }
    }

    // If all voices busy, steal the oldest
    if (slot < 0) {
        float oldest_phase = 0;
        slot = 0;
        for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
            if (audio->voices[i].phase > oldest_phase) {
                oldest_phase = audio->voices[i].phase;
                slot = i;
            }
        }
    }

    AudioVoice* v = &audio->voices[slot];
    v->type = type;
    v->phase = 0;
    v->active = 1;

    switch (type) {
        case SOUND_FOOTSTEP:
            v->duration = 0.12f;
            v->volume = 0.25f;
            // Alternate pitch slightly for left/right foot
            v->frequency = (audio->footstep_alt % 2 == 0) ? 0.0f : 1.0f;
            break;
        case SOUND_JUMP:
            v->duration = 0.2f;
            v->volume = 0.35f;
            v->frequency = 0;
            break;
        case SOUND_LAND:
            v->duration = 0.15f;
            v->volume = 0.3f;
            v->frequency = 0;
            break;
        case SOUND_GUNSHOT:
            v->duration = 0.15f;
            v->volume = 0.4f;
            v->frequency = 0;
            break;
        case SOUND_RELOAD:
            v->duration = 0.8f;
            v->volume = 0.25f;
            v->frequency = 0;
            break;
        default:
            v->active = 0;
            break;
    }

    SDL_UnlockAudioDevice(audio->device);
}

void audio_update_footsteps(AudioState* audio, int is_walking, int is_crouching, float dt) {
    if (!audio->initialized) return;

    if (is_walking) {
        float interval = is_crouching ? 0.55f : 0.35f;
        audio->footstep_timer += dt;
        if (audio->footstep_timer >= interval) {
            audio->footstep_timer -= interval;
            audio_play_sound(audio, SOUND_FOOTSTEP);
            audio->footstep_alt++;
        }
    } else {
        // Reset timer so first step plays immediately when starting to walk
        audio->footstep_timer = 0.3f;
    }
}
