/*
 * MIDI Chords - Transform Pipeline
 * 4-stage degree-space transform pipeline
 */

#pragma once

#include "types.h"

// Apply the full transform pipeline to a DegreeBuffer.
// Combines global params (from v[]) with per-step params.
void applyTransforms(DegreeBuffer* buf, const int16_t* v, int stepIdx, uint32_t& randState);
