#define SDL_MAIN_USE_CALLBACKS 1
#include <math.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *audio_stream = NULL;

const int WIDTH = 800;
const int HEIGHT = 600;

typedef enum {
    AUDIO_WAVE_SINE,
    AUDIO_WAVE_SQUARE,
    AUDIO_WAVE_SAW,
    AUDIO_WAVE_TRIANGLE,
} AudioWaveType;

char *audio_wave_type_to_str(AudioWaveType t) {
    switch (t) {
        case AUDIO_WAVE_SINE:
            return "SINE";
        case AUDIO_WAVE_SQUARE:
            return "SQUARE";
        case AUDIO_WAVE_SAW:
            return "SAW";
        case AUDIO_WAVE_TRIANGLE:
            return "TRIANGLE";
        default:
            SDL_assert(false);
            return "";
    }
}

int sample_rate = 44100;
float BASE_FREQ_A = 440.0f;
float freq = 440.0f;
float phase = 0;
AudioWaveType wave_type = AUDIO_WAVE_TRIANGLE;

static void SDLCALL audio_callback(
    void *userdata,
    SDL_AudioStream *stream,
    int additional_amount,
    int total_amount
) {
    additional_amount = additional_amount / (int) sizeof(float); /* convert from bytes to samples */
    while (additional_amount > 0) {
        float samples[128];
        const int num_samples = SDL_min(additional_amount, SDL_arraysize(samples));

        /* generate a 440Hz pure tone */
        for (int i = 0; i < num_samples; i++) {
            samples[i] = SDL_sinf(2 * SDL_PI_F * phase);
            phase += freq / sample_rate;
            if (phase >= 1.0f) phase -= 1.0f;
        }

        SDL_PutAudioStreamData(stream, samples, num_samples * (int) sizeof(float));
        additional_amount -= num_samples;
    }
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("Learning audio", "0.1", "dev.daniboy.learning-audio");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Learning audio");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, WIDTH);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, HEIGHT);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = sample_rate;
    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, NULL);
    if (!audio_stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_ResumeAudioStreamDevice(audio_stream);

    return SDL_APP_CONTINUE;
}

float map(const float v, const float v_min, const float v_max, const float d_min, const float d_max) {
    float slope = (d_max - d_min) / (v_max - v_min);
    return d_min + slope * (v - v_min);
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (
        event->type == SDL_EVENT_QUIT
        || (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
    ) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        float max = BASE_FREQ_A * 2;
        float f = map(event->motion.x, 0, WIDTH, 0, max);
        freq = f;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_A) {
            wave_type = AUDIO_WAVE_SINE;
        }
        if (event->key.key == SDLK_S) {
            wave_type = AUDIO_WAVE_SQUARE;
        }
        if (event->key.key == SDLK_D) {
            wave_type = AUDIO_WAVE_SAW;
        }
        if (event->key.key == SDLK_F) {
            wave_type = AUDIO_WAVE_TRIANGLE;
        }
    }

    return SDL_APP_CONTINUE;
}

void audio_create_wave_sine(SDL_FPoint *points, float waves) {
    float visual_phase = 0;
    for (int i = 0; i < WIDTH; i++) {
        float y = -100 * SDL_sinf(2 * SDL_PI_F * visual_phase);
        points[i] = (SDL_FPoint){.x = i, .y = y + HEIGHT / 2};
        visual_phase += waves / WIDTH;
        if (visual_phase >= 1.0f) { visual_phase -= 1.0f; }
    }
}

void audio_create_wave_square(SDL_FPoint *points, float waves) {
    float visual_phase = 0;
    for (int i = 0; i < WIDTH; i++) {
        float y = -100 * (SDL_sinf(2 * SDL_PI_F * visual_phase) > 0 ? 1.0f : -1.0f);
        points[i] = (SDL_FPoint){.x = i, .y = y + HEIGHT / 2};
        visual_phase += waves / WIDTH;
        if (visual_phase >= 1.0f) { visual_phase -= 1.0f; }
    }
}

void audio_create_wave_saw(SDL_FPoint *points, float waves) {
    float visual_phase = 0;
    for (int i = 0; i < WIDTH; i++) {
        float y = -100 * visual_phase - 1.0f;
        points[i] = (SDL_FPoint){.x = i, .y = y + HEIGHT / 2};
        visual_phase += waves / WIDTH;
        if (visual_phase >= 1.0f) { visual_phase -= 1.0f; }
    }
}

void audio_create_wave_triangle(SDL_FPoint *points, float waves) {
    float visual_phase = 0;
    for (int i = 0; i < WIDTH; i++) {
        float sample;
        if (visual_phase < 0.25f) {
            sample = 4.0f * visual_phase;  // Rising from 0 to +1
        } else if (visual_phase < 0.75f) {
            sample = 2.0f - 4.0f * visual_phase;  // Falling from +1 to -1
        } else {
            sample = -4.0f + 4.0f * visual_phase;  // Rising from -1 to 0
        }
        float y = -100 * sample;
        points[i] = (SDL_FPoint){.x = i, .y = y + HEIGHT / 2};
        visual_phase += waves / WIDTH;
        if (visual_phase >= 1.0f) { visual_phase -= 1.0f; }
    }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    SDL_RenderDebugTextFormat(renderer, 10, 10, "%.0f %s", freq, audio_wave_type_to_str(wave_type));
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);


    const float N_WAVES_BASE = 4;
    float waves = N_WAVES_BASE * freq / BASE_FREQ_A;
    SDL_FPoint points[WIDTH];
    switch (wave_type) {
        case AUDIO_WAVE_SINE:
            audio_create_wave_sine(points, waves);
            break;
        case AUDIO_WAVE_SQUARE:
            audio_create_wave_square(points, waves);
            break;
        case AUDIO_WAVE_SAW:
            audio_create_wave_saw(points, waves);
            break;
        case AUDIO_WAVE_TRIANGLE:
            audio_create_wave_triangle(points, waves);
            break;
        default:
            SDL_assert(false);
    }

    SDL_RenderPoints(renderer, points, WIDTH);

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
}
