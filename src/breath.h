/*
 * Chordshift - Voice Breathing
 * Cyclical inner-voice movement by ±1–2 scale degrees over time
 */

#pragma once

#include "types.h"

// Apply breathing offsets to inner chord voices in degree space.
// Call after applyTransforms(), before renderChord().
void applyBreathing(DegreeBuffer* buf, const int16_t* v, Chordshift_DTC* dtc, uint32_t& randState);

// Zero breath counter and walk offset.
void resetBreath(Chordshift_DTC* dtc);
