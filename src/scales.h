/*
 * Chordshift - Scale Engine
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
    SCALE_IONIAN = 0,
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

    SCALE_COUNT  // = 11
};

// ============================================================================
// SCALE INTERVAL TABLES (0-based semitone offsets from root)
// ============================================================================

extern const int scaleIntervals[SCALE_COUNT][7];
extern const int scaleSizes[SCALE_COUNT];

// ============================================================================
// SCALE DATA ACCESS
// ============================================================================

struct ScaleData {
    const int* intervals;
    int length;
};

static inline ScaleData getScaleData(int scaleType) {
    ScaleData sd;
    if (scaleType < 0 || scaleType >= SCALE_COUNT) {
        scaleType = 0;  // Default to Ionian
    }
    sd.intervals = scaleIntervals[scaleType];
    sd.length = scaleSizes[scaleType];
    return sd;
}

// ============================================================================
// WHITE KEY LOOKUP TABLE (for input quantization)
// ============================================================================

// Maps pitch class (0-11) to white key index (0-6)
// C=0, C#->0, D=1, D#->1, E=2, F=3, F#->3, G=4, G#->4, A=5, A#->5, B=6
extern const int PC_TO_WHITE_KEY[12];

// Quantize a MIDI note to the given root + scale via white-key mapping.
// White key C always gives root, D gives degree 1, etc.
static inline uint8_t quantizeToScale(uint8_t note, int root, int scaleType) {
    if (scaleType < 0 || scaleType >= SCALE_COUNT) return note;

    int scaleSize = scaleSizes[scaleType];

    int pc = note % 12;
    int octave = note / 12;
    int whiteKeyIdx = PC_TO_WHITE_KEY[pc];

    int extraOctave = whiteKeyIdx / scaleSize;
    int scaleDegree = whiteKeyIdx % scaleSize;

    int outNote = (octave + extraOctave) * 12 + root + scaleIntervals[scaleType][scaleDegree];

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

// Convert scale degree to MIDI note number using pre-fetched scale data.
static inline int degreeToMidiSD(int degree, int root, int octaveBase, const ScaleData& sd) {
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
    return root + octaveBase * 12 + semi;
}

// Convert scale degree to MIDI note number.
static inline int degreeToMidi(int degree, int root, int octaveBase, int scaleType) {
    return degreeToMidiSD(degree, root, octaveBase, getScaleData(scaleType));
}
