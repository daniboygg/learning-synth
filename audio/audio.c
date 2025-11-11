#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

float amplitude = 0.4f;
float freq = 440.0f;
int sample_rate = 44100;

// float t = 0;
float phase = 0.0f;

void audio_callback(void *userdata, Uint8 *stream, int len) {
    float *buffer = (float *) stream;
    size_t num_samples = len / sizeof(float);

    for (int i = 0; i < num_samples; i++) {
        // sinew wave with time, sound artifacts, precision problem?
        // buffer[i] = amplitude * sinf(2 * M_PI * freq * t);
        // t += 1.0f / (float)sample_rate;

        // https://www.desmos.com/calculator/eugiiu4iky
        buffer[i] = amplitude * sinf(2 * M_PI * phase);
        phase += freq / (float) sample_rate;
        // mod(phase, 1) in c with a conditional and a substraction
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

int width = 800;
int height = 600;

int main() {
    // Initialize SDL (video + audio)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "Audio Visualizer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup audio
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = sample_rate;
    want.format = AUDIO_F32; // 32-bit float samples
    want.channels = 1; // mono
    want.samples = 1024; // buffer size (on laptop at 512 cannot produce clean sound)
    want.callback = audio_callback;
    want.userdata = NULL;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Start playing audio
    SDL_PauseAudioDevice(audio_device, 0);

    // Main loop
    bool running = true;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_a && event.key.repeat == 0) {
                    freq = 261.63f;
                }
                if (event.key.keysym.sym == SDLK_w && event.key.repeat == 0) {
                    freq = 277.18f;
                }
                if (event.key.keysym.sym == SDLK_s && event.key.repeat == 0) {
                    freq = 293.66f;
                }
                if (event.key.keysym.sym == SDLK_h && event.key.repeat == 0) {
                    freq = 440.0f;
                }
            }
        }

        // Clear screen (black background)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw waveform (green lines)
        // Generate waveform for visualization (independent of audio samples)
        // Show 2 complete cycles across the screen for clarity
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        #define VISUAL_CYCLES 4.0f  // Number of sine wave cycles to show
        for (int x = 0; x < width - 1; x++) {
            float t1 = (float)x / (float)width * VISUAL_CYCLES;
            float t2 = (float)(x + 1) / (float)width * VISUAL_CYCLES;

            float y1 = amplitude * sinf(2.0f * M_PI * t1);
            float y2 = amplitude * sinf(2.0f * M_PI * t2);

            int screen_y1 = height / 2 + (int)(y1 * 200.0f);
            int screen_y2 = height / 2 + (int)(y2 * 200.0f);

            SDL_RenderDrawLine(renderer, x, screen_y1, x + 1, screen_y2);
        }

        // Present renderer
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
