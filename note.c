#include <assert.h>
#include <stdio.h>
#include <SDL3/SDL.h>

typedef int MidiNote;

const int DEFAULT_MIDI_NOTE = 69; // A = 440 hz = MIDI 69
const char *note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
char note_str_buffer[4] = "A4"; // enough for "C#9\0"

float note_to_freq(const MidiNote note) {
    // notes https://en.wikipedia.org/wiki/Piano_key_frequencies
    const int a440_to_n = 20;
    const int n = note - a440_to_n;
    return SDL_powf(2, ((float) n - 49.0f) / 12.0f) * 440.0f;
}

char *note_to_str(const MidiNote note) {
    int octave = note / 12 - 1;
    int note_index = note % 12;

    snprintf(note_str_buffer, sizeof(note_str_buffer), "%s%d", note_names[note_index], octave);
    return note_str_buffer;
}

// implement last note priority with memory
// VCA volume control will be handled with this
typedef struct {
    MidiNote midi_note;
    float freq;
    float velocity; // from 0.0 to 1.0
} PressedNote;

#define NOTE_MEMORY_STACK_MAX 32

typedef struct {
    int count;
    PressedNote memory[NOTE_MEMORY_STACK_MAX];
} NoteMemory;

NoteMemory note_memory = {0};

void note_memory_push(NoteMemory *nm, const PressedNote note) {
    assert(nm->count < NOTE_MEMORY_STACK_MAX);
    nm->memory[nm->count++] = note;
}

void note_memory_remove(NoteMemory *nm, MidiNote midi_note) {
    for (int i = 0; i < nm->count; i++) {
        if (nm->memory[i].midi_note == midi_note) {
            for (int j = i; j < nm->count - 1; j++) {
                nm->memory[j] = nm->memory[j + 1];
            }
            nm->count--;
            return;
        }
    }
    SDL_Log("MIDI note %d not found in stack\n", midi_note);
    assert(false);
}

const PressedNote *note_memory_peek(const NoteMemory *nm) {
    if (nm->count == 0) {
        return NULL;
    }
    return &nm->memory[nm->count - 1];
}