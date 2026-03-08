/*
 * Chordshift - Transform Pipeline
 * 4-stage degree-space transform pipeline
 */

#pragma once

#include "types.h"

// Apply the full transform pipeline to a DegreeBuffer.
// Combines global params (from v[]) with per-step params.
void applyTransforms(DegreeBuffer* buf, const int16_t* v, int stepIdx, uint32_t& randState);

// Apply only pitch-content transforms (no rotation, reverse, direction).
// Used by UI to preview chord shape + position without ordering effects.
void applyPitchTransforms(DegreeBuffer* buf, const int16_t* v, int stepIdx);

// Apply inversion (move lowest/highest notes across octave boundary).
void applyInversion(DegreeBuffer* buf, int inv, int scaleType);
