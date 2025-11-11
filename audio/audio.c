#define _POSIX_C_SOURCE 200809L
#define SOKOL_IMPL
#include "sokol_audio.h"
#include <math.h>
#include <unistd.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

float amplitude = 0.4f;
float freq = 440.0f;
int sample_rate = 44100;

// float t = 0;
float phase = 0.0f;

void stream_callback(float *buffer, int num_frames, int num_channels) {
    for (int i = 0; i < num_frames; i++) {
        // sinew wave with time, sound artifacts, precision problem?
        // buffer[i] = amplitude * sinf(2 * M_PI * freq * t);
        // t += 1.0f / (float)sample_rate;

        // https://www.desmos.com/calculator/eugiiu4iky
        buffer[i] = amplitude * sinf(2 * M_PI * phase);
        phase += freq / (float)sample_rate;
        // mod(phase, 1) in c with a conditional and a substraction
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

int main() {
    saudio_setup(&(saudio_desc){
        .stream_cb = stream_callback,
        .sample_rate = sample_rate,
        .num_channels = 1,
        // on laptop at 512 cannot produce clean sound
        .buffer_frames = 1024,
    });

    pause();
    saudio_shutdown();
    return 0;
}
