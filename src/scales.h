/*
 * MIDI Chords - Scale Engine
 * Scale interval tables, white-key quantization, and degree-space conversions
 *
 * Extends the midi-looper scales.h with degree-space functions:
 * - midiNoteToDegree(): Convert MIDI note to scale degree offset
 * - degreeToMidi(): Convert scale degree to MIDI note
 * - getScaleData(): Get scale intervals and length
 */

#pragma once

#include <cstdint>

// ============================================================================
// SCALE TYPE ENUM
// ============================================================================

enum ScaleType {
    SCALE_OFF = 0,
    SCALE_IONIAN,
    SCALE_DORIAN,
    SCALE_PHRYGIAN,
    SCALE_LYDIAN,
    SCALE_MIXOLYDIAN,
    SCALE_AEOLIAN,
    SCALE_LOCRIAN,
    SCALE_HARMONIC_MIN,
    SCALE_MELODIC_MIN,
    SCALE_MAJ_PENTATONIC,
    SCALE_MIN_PENTATONIC,

    SCALE_COUNT  // = 12
};

// ============================================================================
// SCALE INTERVAL TABLES (0-based semitone offsets from root)
// ============================================================================

static const int scaleIntervals[][7] = {
    { 0, 2, 4, 5, 7, 9, 11 },  // Ionian (Major)
    { 0, 2, 3, 5, 7, 9, 10 },  // Dorian
    { 0, 1, 3, 5, 7, 8, 10 },  // Phrygian
    { 0, 2, 4, 6, 7, 9, 11 },  // Lydian
    { 0, 2, 4, 5, 7, 9, 10 },  // Mixolydian
    { 0, 2, 3, 5, 7, 8, 10 },  // Aeolian (Natural Minor)
    { 0, 1, 3, 5, 6, 8, 10 },  // Locrian
    { 0, 2, 3, 5, 7, 8, 11 },  // Harmonic Minor
    { 0, 2, 3, 5, 7, 9, 11 },  // Melodic Minor
    { 0, 2, 4, 7, 9, 0, 0 },   // Major Pentatonic (5 notes)
    { 0, 3, 5, 7, 10, 0, 0 },  // Minor Pentatonic (5 notes)
};

static const int scaleSizes[] = {
    7, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5
};

// ============================================================================
// SCALE DATA ACCESS
// ============================================================================

struct ScaleData {
    const int* intervals;
    int length;
};

// Get scale interval data. scaleType must be >= 1 (no SCALE_OFF).
static inline ScaleData getScaleData(int scaleType) {
    ScaleData sd;
    int idx = scaleType - 1;  // Interval table is 0-indexed (no OFF entry)
    if (idx < 0 || idx >= (int)(sizeof(scaleSizes) / sizeof(scaleSizes[0]))) {
        idx = 0;  // Default to Ionian
    }
    sd.intervals = scaleIntervals[idx];
    sd.length = scaleSizes[idx];
    return sd;
}

// ============================================================================
// WHITE KEY LOOKUP TABLE (for input quantization)
// ============================================================================

// Maps pitch class (0-11) to white key index (0-6)
// C=0, C#->0, D=1, D#->1, E=2, F=3, F#->3, G=4, G#->4, A=5, A#->5, B=6
static const int PC_TO_WHITE_KEY[12] = {
    0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6
};

// Quantize a MIDI note to the given root + scale via white-key mapping.
// White key C always gives root, D gives degree 1, etc.
static inline uint8_t quantizeToScale(uint8_t note, int root, int scaleType) {
    if (scaleType <= SCALE_OFF || scaleType >= SCALE_COUNT) return note;

    int scaleIdx = scaleType - 1;
    int scaleSize = scaleSizes[scaleIdx];

    int pc = note % 12;
    int octave = note / 12;
    int whiteKeyIdx = PC_TO_WHITE_KEY[pc];

    int extraOctave = whiteKeyIdx / scaleSize;
    int scaleDegree = whiteKeyIdx % scaleSize;

    int outNote = (octave + extraOctave) * 12 + root + scaleIntervals[scaleIdx][scaleDegree];

    if (outNote < 0) outNote = 0;
    if (outNote > 127) outNote = 127;

    return (uint8_t)outNote;
}

// ============================================================================
// DEGREE-SPACE CONVERSION FUNCTIONS
// ============================================================================

// Convert MIDI note to degree offset using root-relative nearest-degree matching.
// The note should already be quantized to the scale (via quantizeToScale).
// Algorithm: compute relative = (note - root) mod 12, find closest scale degree.
static inline int midiNoteToDegree(uint8_t note, int root, int scaleType) {
    ScaleData sd = getScaleData(scaleType);

    // Get the octave and relative semitone from root
    int diff = (int)note - root;
    int octave, relative;

    // Floor division for negative values
    if (diff >= 0) {
        octave = diff / 12;
        relative = diff % 12;
    } else {
        // For negative: floor division
        octave = (diff - 11) / 12;  // floor division
        relative = diff - octave * 12;
    }

    // Find the nearest scale degree for this relative semitone value
    int bestDegree = 0;
    int bestDist = 999;
    for (int d = 0; d < sd.length; d++) {
        int dist = relative - sd.intervals[d];
        if (dist < 0) dist = -dist;
        if (dist < bestDist) {
            bestDist = dist;
            bestDegree = d;
        }
    }

    return octave * sd.length + bestDegree;
}

// Convert scale degree to MIDI note number.
// Formula: oct = floor(d/scaleLen); deg = mod(d,scaleLen);
//          semi = 12*oct + scale[deg]; midi = root + octaveBase*12 + semi
static inline int degreeToMidi(int degree, int root, int octaveBase, int scaleType) {
    ScaleData sd = getScaleData(scaleType);

    int oct, deg;
    // Floor division and modulo for negative degrees
    if (degree >= 0) {
        oct = degree / sd.length;
        deg = degree % sd.length;
    } else {
        // Floor division for negatives
        oct = (degree - (sd.length - 1)) / sd.length;
        deg = degree - oct * sd.length;
    }

    int semi = 12 * oct + sd.intervals[deg];
    int midi = root + octaveBase * 12 + semi;

    return midi;
}
