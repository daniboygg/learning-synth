#define SDL_MAIN_USE_CALLBACKS 1
#include <assert.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "oscillator.c"
#include "notes.c"
#include "portmidi.h"
#include "porttime.h"

// Window and rendering
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
const int WIDTH = 800;
const int HEIGHT = 600;

// FPS tracking
Uint64 last_frame_time = 0;
float last_update_fps = 0;
float fps = 0.0f;

// Audio
SDL_AudioStream *audio_stream = NULL;
int sample_rate = 44100;
float BASE_FREQ_A = 440.0f;
float volume = 1.0f;
float audio_phase = 0;

Oscillator oscillator = {0};

// MIDI
PortMidiStream *midi = NULL;
#define INPUT_BUFFER_SIZE 100
#define TIME_PROC ((int32_t (*)(void *)) Pt_Time)
#define TIME_INFO NULL
#define MIDI_DEVICE_ID 5
#define MIDI_EVENT_BUFFER_SIZE 32
PmEvent midi_event_buffer[MIDI_EVENT_BUFFER_SIZE];
MidiNote current_midi_note = DEFAULT_MIDI_NOTE;


void SDLCALL audio_callback(
    void *userdata,
    SDL_AudioStream *stream,
    int additional_amount,
    int total_amount
) {
    const PressedNote *last_note = note_memory_peek(&note_memory);
    if (last_note == NULL) {
        return;
    }

    float amplitude = last_note->velocity * volume;

    additional_amount = additional_amount / (int) sizeof(float); /* convert from bytes to samples */
    while (additional_amount > 0) {
        float samples[128];
        const int num_samples = SDL_min(additional_amount, SDL_arraysize(samples));

        for (int i = 0; i < num_samples; i++) {
            samples[i] = oscillator_next_point(oscillator, amplitude, audio_phase);
            audio_phase += last_note->freq / (float) sample_rate;
            if (audio_phase >= 1.0f) audio_phase -= 1.0f;
        }

        SDL_PutAudioStreamData(stream, samples, num_samples * (int) sizeof(float));
        additional_amount -= num_samples;
    }
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_SetHint(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT, "1");

    SDL_SetAppMetadata("Learning audio", "0.1", "dev.daniboy.learning-audio");

    // window creation
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
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetRenderVSync(renderer, 1)) {
        SDL_Log("Couldn't vsync renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    const bool presentation = SDL_SetRenderLogicalPresentation(
        renderer,
        WIDTH,
        HEIGHT,
        SDL_LOGICAL_PRESENTATION_LETTERBOX
    );
    if (!presentation) {
        SDL_Log("Couldn't create render pressentation: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // audio stream creation
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


    // midi process creation
    Pm_Initialize();
    Pt_Start(1, NULL, NULL);

    // Open the MIDI input device
    PmError err = Pm_OpenInput(
        &midi,
        MIDI_DEVICE_ID,
        NULL,
        INPUT_BUFFER_SIZE,
        TIME_PROC,
        TIME_INFO
    );

    if (err != pmNoError) {
        SDL_Log("Couldn't open MIDI device %d: %s", MIDI_DEVICE_ID, Pm_GetErrorText(err));
        SDL_Log("Available MIDI input devices:");
        const int num_devices = Pm_CountDevices();
        for (int i = 0; i < num_devices; i++) {
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            if (info->input) {
                SDL_Log("  %d: %s - %s", i, info->interf, info->name);
            }
        }
        Pm_Terminate();
        return SDL_APP_FAILURE;
    }

    SDL_Log("MIDI device %d opened successfully", MIDI_DEVICE_ID);

    // create oscillators
    oscillator = oscillator_init(WAVE_SINE);

    return SDL_APP_CONTINUE;
}

float map(const float v, const float v_min, const float v_max, const float d_min, const float d_max) {
    const float slope = (d_max - d_min) / (v_max - v_min);
    return d_min + slope * (v - v_min);
}

float clamp(const float v, const float v_min, const float v_max) {
    if (v < v_min) { return v_min; }
    if (v > v_max) { return v_max; }
    return v;
}

float initial_x = -1;

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (
        event->type == SDL_EVENT_QUIT
        || (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
    ) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_1) {
            oscillator.wave_type = WAVE_SINE;
        }
        if (event->key.key == SDLK_2) {
            oscillator.wave_type = WAVE_SQUARE;
        }
        if (event->key.key == SDLK_3) {
            oscillator.wave_type = WAVE_SAW;
        }
        if (event->key.key == SDLK_4) {
            oscillator.wave_type = WAVE_TRIANGLE;
        }
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_UP) {
            // for easy debugging
        }
        if (event->key.key == SDLK_DOWN) {
            // for easy debugging
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // fps display - update once per second
    const Uint64 current_time = SDL_GetTicks();
    const float delta_ms = (float) (current_time - last_frame_time);
    if (delta_ms > 0.0f && last_update_fps > 1000.0f) {
        fps = 1000.0f / delta_ms;
        last_update_fps = 0.0f;
    } else {
        last_update_fps += delta_ms;
    }
    last_frame_time = current_time;
    SDL_SetRenderDrawColorFloat(renderer, 1, 1, 1, 0.6f);
    SDL_RenderDebugTextFormat(renderer, (float) WIDTH - 75, 10, "%.0f FPS", fps);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    // freq display
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugTextFormat(renderer, 10, 10, "%.0f %s", oscillator.freq, waves_type_to_str(oscillator.wave_type));
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    // note display
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugTextFormat(renderer, 150, 10, "NOTE: %s", note_to_str(current_midi_note));
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    // Volume display
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderDebugTextFormat(renderer, 230, 10, "VOLUME: %d", (int)(volume * 100));
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    // process MIDI events
    if (midi) {
        const int num_events = Pm_Read(midi, midi_event_buffer, 32);

        for (int i = 0; i < num_events; i++) {
            const PmMessage msg = midi_event_buffer[i].message;
            const uint8_t status = Pm_MessageStatus(msg);

            if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) {
                const MidiNote note = Pm_MessageData1(msg);
                const uint8_t velocity = Pm_MessageData2(msg);

                // Note On event 0x90
                if ((status & 0xF0) == 0x90 && velocity > 0) {
                    current_midi_note = note;
                    const float note_freq = note_to_freq(current_midi_note);
                    const float note_velocity = map(velocity, 0.0f, 255.0f, 0.0f, 1.0f);
                    const PressedNote pressed_note = {
                        .freq = note_freq,
                        .midi_note = current_midi_note,
                        .velocity = note_velocity
                    };
                    note_memory_push(&note_memory, pressed_note);
                }

                // Note Off event 0x80
                if ((status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && velocity == 0)) {
                    note_memory_remove(&note_memory, note);
                }
            } else if ((status & 0xF0) == 0xB0) {
                // Control change value 0xB0
                const uint8_t cc_number = Pm_MessageData1(msg);
                const uint8_t cc_value = Pm_MessageData2(msg);
                // printf("CC: number %d, value %d\n", cc_number, cc_value);

                if (cc_number == 93) {
                    // knob 5
                    float value = cc_value;
                    oscillator.square_pulse_width = map(value, 0.0f, 127.0f, 0.0f, 1.0f);
                }

                if (cc_number == 17) {
                    // fader 4
                    float value = cc_value;
                    volume = map(value, 0.0f, 127.0f, 0.0f, 1.0f);
                }
            }
        }
    }

    // render
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    const float N_WAVES_BASE = 4;
    const float waves = N_WAVES_BASE * oscillator.freq / BASE_FREQ_A;
    SDL_FPoint points[WIDTH];
    float visual_phase = 0;
    for (int i = 0; i < WIDTH; i++) {
        float y = oscillator_next_point(oscillator, 100, visual_phase);
        y = -y; // correct for graphic coordinates, they increase from top to bottom
        y = y + (float) HEIGHT / 2;
        points[i] = (SDL_FPoint){.x = (float) i, .y = y};

        visual_phase += waves / (float) WIDTH;
        if (visual_phase >= 1.0f) { visual_phase -= 1.0f; }
    }
    SDL_RenderPoints(renderer, points, WIDTH);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    // Clean up MIDI
    if (midi) {
        Pm_Close(midi);
    }
    Pm_Terminate();

    // Clean up SDL
    if (audio_stream) {
        SDL_DestroyAudioStream(audio_stream);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}
