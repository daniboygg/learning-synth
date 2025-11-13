#include "greatest.h"
#include "notes.c"

TEST note_to_freq_a440(void) {
    // MIDI note 69 is A4 = 440 Hz
    float freq = note_to_freq(69);
    ASSERT_IN_RANGE(440.0f, freq, 0.01f);
    PASS();
}

TEST note_to_freq_middle_c(void) {
    // MIDI note 60 is C4 (middle C) = 261.63 Hz
    float freq = note_to_freq(60);
    ASSERT_IN_RANGE(261.63f, freq, 0.01f);
    PASS();
}

TEST note_to_freq_c0(void) {
    // MIDI note 12 is C0 = 16.35 Hz
    float freq = note_to_freq(12);
    ASSERT_IN_RANGE(16.35f, freq, 0.01f);
    PASS();
}

TEST note_to_str_a4(void) {
    char *str = note_to_str(69);
    ASSERT_STR_EQ("A4", str);
    PASS();
}

TEST note_to_str_middle_c(void) {
    char *str = note_to_str(60);
    ASSERT_STR_EQ("C4", str);
    PASS();
}

TEST note_to_str_c_sharp(void) {
    char *str = note_to_str(61);
    ASSERT_STR_EQ("C#4", str);
    PASS();
}

TEST note_to_str_c0(void) {
    char *str = note_to_str(12);
    ASSERT_STR_EQ("C0", str);
    PASS();
}

TEST note_to_str_g9(void) {
    char *str = note_to_str(127);
    ASSERT_STR_EQ("G9", str);
    PASS();
}

TEST note_to_str_all_notes_in_octave(void) {
    // Test all 12 notes in octave 4
    const char *expected[] = {
        "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4"
    };
    for (int i = 0; i < 12; i++) {
        char *str = note_to_str(60 + i);
        ASSERT_STR_EQ(expected[i], str);
    }
    PASS();
}

TEST note_memory_push_single(void) {
    NoteMemory nm = {0};
    PressedNote note = {.midi_note = 60, .freq = 261.63f, .velocity = 0.8f};

    note_memory_push(&nm, note);

    ASSERT_EQ(1, nm.count);
    ASSERT_EQ(60, nm.memory[0].midi_note);
    ASSERT_IN_RANGE(261.63f, nm.memory[0].freq, 0.01f);
    ASSERT_IN_RANGE(0.8f, nm.memory[0].velocity, 0.01f);
    PASS();
}

TEST note_memory_push_multiple(void) {
    NoteMemory nm = {0};

    for (int i = 0; i < 5; i++) {
        PressedNote note = {.midi_note = 60 + i, .freq = 261.63f + i, .velocity = 0.5f};
        note_memory_push(&nm, note);
    }

    ASSERT_EQ(5, nm.count);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(60 + i, nm.memory[i].midi_note);
    }
    PASS();
}

TEST note_memory_peek_empty(void) {
    NoteMemory nm = {0};
    const PressedNote *note = note_memory_peek(&nm);
    ASSERT_EQ(NULL, note);
    PASS();
}

TEST note_memory_peek_single(void) {
    NoteMemory nm = {0};
    PressedNote note = {.midi_note = 69, .freq = 440.0f, .velocity = 1.0f};

    note_memory_push(&nm, note);

    const PressedNote *peeked = note_memory_peek(&nm);
    ASSERT(peeked != NULL);
    ASSERT_EQ(69, peeked->midi_note);
    ASSERT_IN_RANGE(440.0f, peeked->freq, 0.01f);
    ASSERT_IN_RANGE(1.0f, peeked->velocity, 0.01f);
    PASS();
}

TEST note_memory_peek_returns_last(void) {
    NoteMemory nm = {0};

    for (int i = 0; i < 3; i++) {
        PressedNote note = {.midi_note = 60 + i, .freq = 261.63f + i, .velocity = 0.5f};
        note_memory_push(&nm, note);
    }

    const PressedNote *peeked = note_memory_peek(&nm);
    ASSERT(peeked != NULL);
    ASSERT_EQ(62, peeked->midi_note); // Last pushed note
    PASS();
}

TEST note_memory_remove_single(void) {
    NoteMemory nm = {0};
    PressedNote note = {.midi_note = 60, .freq = 261.63f, .velocity = 0.8f};

    note_memory_push(&nm, note);
    note_memory_remove(&nm, 60);

    ASSERT_EQ(0, nm.count);
    PASS();
}

TEST note_memory_remove_from_middle(void) {
    NoteMemory nm = {0};

    // Push notes 60, 62, 64
    for (int i = 0; i < 3; i++) {
        PressedNote note = {.midi_note = 60 + i * 2, .freq = 261.63f, .velocity = 0.5f};
        note_memory_push(&nm, note);
    }

    // Remove middle note (62)
    note_memory_remove(&nm, 62);

    ASSERT_EQ(2, nm.count);
    ASSERT_EQ(60, nm.memory[0].midi_note);
    ASSERT_EQ(64, nm.memory[1].midi_note);
    PASS();
}

TEST note_memory_remove_from_beginning(void) {
    NoteMemory nm = {0};

    for (int i = 0; i < 3; i++) {
        PressedNote note = {.midi_note = 60 + i, .freq = 261.63f, .velocity = 0.5f};
        note_memory_push(&nm, note);
    }

    note_memory_remove(&nm, 60);

    ASSERT_EQ(2, nm.count);
    ASSERT_EQ(61, nm.memory[0].midi_note);
    ASSERT_EQ(62, nm.memory[1].midi_note);
    PASS();
}

TEST note_memory_remove_from_end(void) {
    NoteMemory nm = {0};

    for (int i = 0; i < 3; i++) {
        PressedNote note = {.midi_note = 60 + i, .freq = 261.63f, .velocity = 0.5f};
        note_memory_push(&nm, note);
    }

    note_memory_remove(&nm, 62);

    ASSERT_EQ(2, nm.count);
    ASSERT_EQ(60, nm.memory[0].midi_note);
    ASSERT_EQ(61, nm.memory[1].midi_note);
    PASS();
}

TEST note_memory_max_capacity(void) {
    NoteMemory nm = {0};

    // Fill to capacity
    for (int i = 0; i < NOTE_MEMORY_STACK_MAX; i++) {
        PressedNote note = {.midi_note = i, .freq = 100.0f + i, .velocity = 0.5f};
        note_memory_push(&nm, note);
    }

    ASSERT_EQ(NOTE_MEMORY_STACK_MAX, nm.count);
    ASSERT_EQ(NOTE_MEMORY_STACK_MAX - 1, note_memory_peek(&nm)->midi_note);
    PASS();
}

SUITE(note_to_freq_suite) {
    RUN_TEST(note_to_freq_a440);
    RUN_TEST(note_to_freq_middle_c);
    RUN_TEST(note_to_freq_c0);
}

SUITE(note_to_str_suite) {
    RUN_TEST(note_to_str_a4);
    RUN_TEST(note_to_str_middle_c);
    RUN_TEST(note_to_str_c_sharp);
    RUN_TEST(note_to_str_c0);
    RUN_TEST(note_to_str_g9);
    RUN_TEST(note_to_str_all_notes_in_octave);
}

SUITE(note_memory_suite) {
    RUN_TEST(note_memory_push_single);
    RUN_TEST(note_memory_push_multiple);
    RUN_TEST(note_memory_peek_empty);
    RUN_TEST(note_memory_peek_single);
    RUN_TEST(note_memory_peek_returns_last);
    RUN_TEST(note_memory_remove_single);
    RUN_TEST(note_memory_remove_from_middle);
    RUN_TEST(note_memory_remove_from_beginning);
    RUN_TEST(note_memory_remove_from_end);
    RUN_TEST(note_memory_max_capacity);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(note_to_freq_suite);
    RUN_SUITE(note_to_str_suite);
    RUN_SUITE(note_memory_suite);
    GREATEST_MAIN_END();
}
