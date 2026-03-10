/*
 * Chordshift - Harmonic Drift
 * Probabilistic runtime chord substitution via degree-space transpose
 */

#pragma once

#include "types.h"

// Evaluate drift: increment counter, possibly update driftOffset[] for one step.
// Call once per clock tick that advances the step.
void evaluateDrift(ChordshiftAlgorithm* alg);

// Transpose all degrees in buf by the given offset.
void applyDrift(DegreeBuffer* buf, int8_t offset);

// Zero all drift state (offsets, counter, focus).
void resetDrift(Chordshift_DTC* dtc);
