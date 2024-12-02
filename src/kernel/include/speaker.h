#ifndef SPEAKER_H_
#define SPEAKER_H_

#include "pic.h"
#include "x86.h"

#include <time.h>

#define PC_SPEAKER_PORT 0x61

// TODO: Mess with PWM and get it to work for pc speaker sound?
// ...

// Frequency rates to divide PIT default rate of 1193182mhz by to get a given note
// b = flat
// s = sharp (#)
typedef enum {
    C2  = 18356,
    Cs2 = 17292,
    Db2 = Cs2,
    D2  = 16344,
    Ds2 = 15297,
    Eb2 = Ds2,
    E2  = 14551,
    F2  = 13714,
    Fs2 = 12829,
    Gb2 = Fs2,
    G2  = 12175,
    Gs2 = 11472,
    Ab2 = Gs2,
    A2  = 10847,
    As2 = 10198,
    Bb2 = As2,
    B2  = 9700,
    C3  = 9121,
    Cs3 = 8609,
    Db3 = Cs3,
    D3  = 8126,
    Ds3 = 7670,
    Eb3 = Ds3,
    E3  = 7239,
    F3  = 6833,
    Fs3 = 6449,
    Gb3 = Fs3,
    G3  = 6087,
    Gs3 = 5746,
    Ab3 = Gs3,
    A3  = 5423,
    As3 = 5119,
    Bb3 = As3,
    B3  = 4831,
    C4  = 4560,  // Middle C, ~261.63hz (actual from PIT will be ~261.66hz)
    Cs4 = 4304,
    Db4 = Cs4,
    D4  = 4063,
    Ds4 = 3834,
    Eb4 = Ds4,
    E4  = 3619,
    F4  = 3416,
    Fs4 = 3224,
    Gb4 = Fs4,
    G4  = 3043,
    Gs4 = 2873,
    Ab4 = Gs4,
    A4  = 2711,  // standard A tuning, ~440hz
    As4 = 2559,
    Bb4 = As4,
    B4  = 2415,
    C5  = 2280,
    Cs5 = 2152,
    Db5 = Cs5,
    D5  = 2031,
    Ds5 = 1918,
    Eb5 = Ds5,
    E5  = 1810,
    F5  = 1708,
    Fs5 = 1612,
    Gb5 = Fs5,
    G5  = 1522,
    Gs5 = 1437,
    Ab5 = Gs5,
    A5  = 1356,
    As5 = 1280,
    Bb5 = As5,
    B5  = 1208,
    C6  = 1140
} note_freq_t;

// Time signature lower number - what type of note gets the beat
typedef enum {
    WHOLE     = 1,
    HALF      = 2,
    QUARTER   = 4,
    EIGTH     = 8,
    SIXTEENTH = 16,
    THIRTY2ND = 32,
} beat_type_t;


extern uint32_t bpm_ms;
extern uint32_t whole_note_duration;
extern uint32_t half_note_duration;
extern uint32_t quarter_note_duration;
extern uint32_t eigth_note_duration;
extern uint32_t sixteenth_note_duration;
extern uint32_t thirty2nd_note_duration;


void enable_pc_speaker();
void disable_pc_speaker();

void play_note(const note_freq_t note, const uint32_t ms_duration);
void rest(const uint32_t ms_duration);

void set_bpm(const uint32_t bpm);
void set_time_signature(const uint8_t beats_per_measure, const beat_type_t beat_type);

void whole_note(const note_freq_t note);
void half_note(const note_freq_t note);
void quarter_note(const note_freq_t note);
void eigth_note(const note_freq_t note);
void sixteenth_note(const note_freq_t note);
void thirty2nd_note(const note_freq_t note);

void whole_rest(void);
void half_rest(void);
void quarter_rest(void);
void eigth_rest(void);
void sixteenth_rest(void);
void thirty2nd_rest(void);

void dotted_eigth_note(const note_freq_t note);
void eigth_triplet(const note_freq_t note1, const note_freq_t note2, const note_freq_t note3);

#endif