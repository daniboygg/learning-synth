#include <SDL3/SDL.h>

typedef struct {
    // 4 buffer stages for -24dB/octave
    float buf0;
    float buf1;
    float buf2;
    float buf3;
    float cutoff; // 0.01 to 1.00
} FilterLowpass;

FilterLowpass filter_lowpass_init() {
    FilterLowpass filter = {
        .buf0 = 0.0f,
        .buf1 = 0.0f,
        .buf2 = 0.0f,
        .buf3 = 0.0f,
        .cutoff = 1.0f
    };
    return filter;
}

void filter_lowpass_set_cutoff(FilterLowpass *filter, float cutoff) {
    filter->cutoff = SDL_clamp(cutoff, 0.01f, 1.0f);
}

// Process a single sample through the filter
float filter_lowpass_process(FilterLowpass *filter, float input) {
    // Bypass filter when fully open
    if (filter->cutoff > 0.99f) {
        return input;
    }

    float calculated_cutoff = SDL_clamp(filter->cutoff, 0.01f, 1.0f);

    filter->buf0 += calculated_cutoff * (input - filter->buf0);
    filter->buf1 += calculated_cutoff * (filter->buf0 - filter->buf1);
    filter->buf2 += calculated_cutoff * (filter->buf1 - filter->buf2);
    filter->buf3 += calculated_cutoff * (filter->buf2 - filter->buf3);

    return filter->buf3;
}
