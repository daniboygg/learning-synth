/*
    Wave functions, checks desmos for the plot of the signals genrated
    https://www.desmos.com/calculator/eugiiu4iky
*/
#include <SDL3/SDL.h>
#include <assert.h>

typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_TRIANGLE,
} WavesType;

char *waves_type_to_str(WavesType t) {
    switch (t) {
        case WAVE_SINE:
            return "SINE";
        case WAVE_SQUARE:
            return "SQUARE";
        case WAVE_SAW:
            return "SAW";
        case WAVE_TRIANGLE:
            return "TRIANGLE";
        default:
            assert(false);
    }
}

float waves_sine(float amplitude, float phase) {
    float y = amplitude * SDL_sinf(2 * SDL_PI_F * phase);
    return y;
}

/**
 * @param pulse_with 0 = 50% duty, 1 = 0% duty
 */
float waves_square(float amplitude, float phase, float pulse_with) {
    assert(pulse_with >= 0);
    assert(pulse_with <= 1);
    float y = amplitude * (SDL_sinf(2 * SDL_PI_F * phase) > pulse_with ? 1.0f : -1.0f);
    return y;
}

float waves_saw(float amplitude, float phase) {
    float y = -amplitude + 2 * amplitude * phase;
    return y;
}

float waves_triangle(float amplitude, float phase) {
    float y = amplitude - 4 * amplitude * SDL_fabsf(phase - 0.5);
    return y;
}


typedef struct {
    WavesType wave_type;
    float freq;
    float initial_phase;
    float square_pulse_width;
} Oscillator;

Oscillator oscillator_init(WavesType wave_type) {
    Oscillator oscillator = {.freq = 440.0f, .wave_type = wave_type};
    return oscillator;
}

float oscillator_next_point(Oscillator oscillator, float amplitude, float phase) {
    switch (oscillator.wave_type) {
        case WAVE_SINE:
            return waves_sine(amplitude, phase);
        case WAVE_SQUARE:
            return waves_square(amplitude, phase, oscillator.square_pulse_width);
        case WAVE_SAW:
            return waves_saw(amplitude, phase);
        case WAVE_TRIANGLE:
            return waves_triangle(amplitude, phase);
        default:
            assert(false);
    }
}
