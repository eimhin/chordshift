/*
 * Chordshift - Chord Extensions
 * Voicing-aware diatonic chord extension (7th, 9th, 11th, 13th)
 */

#pragma once

#include "types.h"

// Add scored extension candidates (7th, 9th, 11th, 13th) to a DegreeBuffer.
// Operates in degree space. Uses Ext Depth and Ext Color global params.
void applyExtensions(DegreeBuffer* buf, const int16_t* v);
