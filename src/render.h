/*
 * MIDI Chords - Render Pipeline
 * Degree -> MIDI conversion + articulation
 */

#pragma once

#include "types.h"

// Render a transformed DegreeBuffer into a RenderedChord.
// Applies degree->MIDI conversion, velocity curves, strum delays, ratchet, gate length.
void renderChord(RenderedChord* out, const DegreeBuffer* buf, const int16_t* v,
                 int stepIdx, float stepDuration, uint32_t& randState);
