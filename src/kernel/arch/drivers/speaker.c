/*
 * pc_speaker.h: Definitions and functions for emulated (maybe hardware?) pc speaker
 * Code took from: https://github.com/queso-fuego/amateuros/blob/master/include/sound/pc_speaker.h
 */
#include "../../include/speaker.h"


static uint32_t bpm_ms                  = 0;
static uint32_t whole_note_duration     = 0;
static uint32_t half_note_duration      = 0;
static uint32_t quarter_note_duration   = 0;
static uint32_t eigth_note_duration     = 0;
static uint32_t sixteenth_note_duration = 0;
static uint32_t thirty2nd_note_duration = 0;


void set_pit_channel_mode_frequency(const uint8_t channel, const uint8_t operating_mode, const uint16_t frequency) {
    // Invalid input
    if (channel > 2) return;

    asm("cli");

    /* PIT I/O Ports:
     * 0x40 - channel 0     (read/write) 
     * 0x41 - channel 1     (read/write) 
     * 0x42 - channel 2     (read/write) 
     * 0x43 - Mode/Command  (write only) 
     *
     * 0x43 Command register value bits (1 byte):
     * 7-6 select channel:
     *      00 = channel 0
     *      01 = channel 1
     *      10 = channel 2
     *      11 = read-back command
     * 5-4 access mode:
     *      00 = latch count value
     *      01 = lobyte only
     *      10 = hibyte only
     *      11 = lobyte & hibyte
     * 3-1 operating mode:
     *      000 = mode 0 (interrupt on terminal count)
     *      001 = mode 1 (hardware re-triggerable one-shot)
     *      010 = mode 2 (rate generator)
     *      011 = mode 3 (square wave generator)
     *      100 = mode 4 (software triggered strobe)
     *      101 = mode 5 (hardware triggered strobe)
     *      110 = mode 6 (rate generator, same as 010)
     *      111 = mode 7 (square wave generator, same as 011)
     * 0  BCD/Binary mode:
     *      0 = 16bit binary
     *      1 = 4-digit BCD (x86 does not use this!)
     */

    // Send the command byte, always sending lobyte/hibyte for access mode
    i386_outb(0x43, (channel << 6) | (0x3 << 4) | (operating_mode << 1));

    // Send the frequency divider to the input channel
    i386_outb(0x40 + channel, (uint8_t)frequency);           // Low byte
    i386_outb(0x40 + channel, (uint8_t)(frequency >> 8));    // High byte

    asm("sti");
}

// Enable PC Speaker
void enable_pc_speaker() {
    uint8_t temp = i386_inb(PC_SPEAKER_PORT);
    i386_outb(PC_SPEAKER_PORT, temp | 3);  // Set first 2 bits to turn on speaker
}

// Disable PC Speaker
void disable_pc_speaker() {
    uint8_t temp = i386_inb(PC_SPEAKER_PORT);
    i386_outb(PC_SPEAKER_PORT, temp & 0xFC);  // Clear first 2 bits to turn off speaker
}

void play_note(const note_freq_t note, const uint32_t ms_duration) {
    set_pit_channel_mode_frequency(2, 3, note);
    sleep_ms(ms_duration);
}

// Rest for a given duration
void rest(const uint32_t ms_duration) {
    set_pit_channel_mode_frequency(2, 3, 40);
    sleep_ms(ms_duration);
}

void set_bpm(const uint32_t bpm) {
    bpm_ms = 60000 / bpm;
}

void set_time_signature(const uint8_t beats_per_measure, const beat_type_t beat_type) {
    (void)beats_per_measure;
    switch(beat_type) {
        case WHOLE:
            whole_note_duration     = bpm_ms;
            half_note_duration      = bpm_ms / 2;
            quarter_note_duration   = bpm_ms / 4;
            eigth_note_duration     = bpm_ms / 8;
            sixteenth_note_duration = bpm_ms / 16;
            thirty2nd_note_duration = bpm_ms / 32;
            break;

        case HALF:
            whole_note_duration     = bpm_ms * 2;
            half_note_duration      = bpm_ms;
            quarter_note_duration   = bpm_ms / 2;
            eigth_note_duration     = bpm_ms / 4;
            sixteenth_note_duration = bpm_ms / 8;
            thirty2nd_note_duration = bpm_ms / 16;
            break;

        case QUARTER:
            whole_note_duration     = bpm_ms * 4;
            half_note_duration      = bpm_ms * 2;
            quarter_note_duration   = bpm_ms;
            eigth_note_duration     = bpm_ms / 2;
            sixteenth_note_duration = bpm_ms / 4;
            thirty2nd_note_duration = bpm_ms / 8;
            break;

        case EIGTH:
            whole_note_duration     = bpm_ms * 8;
            half_note_duration      = bpm_ms * 4;
            quarter_note_duration   = bpm_ms * 2;
            eigth_note_duration     = bpm_ms;
            sixteenth_note_duration = bpm_ms / 2;
            thirty2nd_note_duration = bpm_ms / 4;
            break;

        case SIXTEENTH:
            whole_note_duration     = bpm_ms * 16;
            half_note_duration      = bpm_ms * 8;
            quarter_note_duration   = bpm_ms * 4;
            eigth_note_duration     = bpm_ms * 2;
            sixteenth_note_duration = bpm_ms;
            thirty2nd_note_duration = bpm_ms / 2;
            break;
            
        case THIRTY2ND:
            whole_note_duration     = bpm_ms * 32;
            half_note_duration      = bpm_ms * 16;
            quarter_note_duration   = bpm_ms * 8;
            eigth_note_duration     = bpm_ms * 4;
            sixteenth_note_duration = bpm_ms * 2;
            thirty2nd_note_duration = bpm_ms;
            break;
    }
}

void whole_note(const note_freq_t note) {
    play_note(note, whole_note_duration);
}

void half_note(const note_freq_t note) {
    play_note(note, half_note_duration);
}

void quarter_note(const note_freq_t note) {
    play_note(note, quarter_note_duration);
}

void eigth_note(const note_freq_t note) {
    play_note(note, eigth_note_duration);
}

void sixteenth_note(const note_freq_t note) {
    play_note(note, sixteenth_note_duration);
}

void thirty2nd_note(const note_freq_t note) {
    play_note(note, thirty2nd_note_duration);
}

void whole_rest(void) {
    rest(whole_note_duration);
}

void half_rest(void) {
    rest(half_note_duration);
}

void quarter_rest(void) {
    rest(quarter_note_duration);
}

void eigth_rest(void) {
    rest(eigth_note_duration);
}

void sixteenth_rest(void) {
    rest(sixteenth_note_duration);
}

void thirty2nd_rest(void) {
    rest(thirty2nd_note_duration);
}

void dotted_eigth_note(const note_freq_t note) {
    play_note(note, eigth_note_duration + sixteenth_note_duration);
}

void eigth_triplet(const note_freq_t note1, const note_freq_t note2, const note_freq_t note3) {
    uint32_t temp = quarter_note_duration / 3;

    play_note(note1, temp);
    play_note(note2, temp);
    play_note(note3, temp);
}

