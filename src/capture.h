/*
 * Chordshift - Capture Pipeline
 * MIDI input -> degree-space chord capture
 */

#pragma once

#include "types.h"

// Shared held-note tracking helpers (used by capture and midiMessage)
static inline void trackHeldNote(ChordshiftAlgorithm* alg, uint8_t note, uint8_t velocity) {
    if (alg->captureNotes[note] == 0) {
        alg->captureNotes[note] = 1;
        alg->dtc->captureCount++;
    }
    alg->dtc->inputVel = velocity;
}

static inline void untrackHeldNote(ChordshiftAlgorithm* alg, uint8_t note) {
    if (alg->captureNotes[note]) {
        alg->captureNotes[note] = 0;
        if (alg->dtc->captureCount > 0) alg->dtc->captureCount--;
    }
}

// Called from midiMessage() for note on/off events
void captureNoteOn(ChordshiftAlgorithm* alg, uint8_t note, uint8_t velocity);
void captureNoteOff(ChordshiftAlgorithm* alg, uint8_t note);
