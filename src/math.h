/*
 * MIDI Chords - Math Utilities
 * Basic math functions and safe array access helpers
 */

#pragma once

#include "config.h"

// ============================================================================
// MATH UTILITIES
// ============================================================================

static inline int clamp(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline float clampf(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// ============================================================================
// SAFE ARRAY ACCESS HELPERS
// ============================================================================

static inline int safeStepIndex(int idx) {
    if (idx < 0) return 0;
    if (idx >= NUM_STEPS) return NUM_STEPS - 1;
    return idx;
}

static inline int safeNoteIndex(int idx) {
    if (idx < 0) return 0;
    if (idx >= 128) return 127;
    return idx;
}

static inline int safeRenderIndex(int idx) {
    if (idx < 0) return 0;
    if (idx >= MAX_RENDER_NOTES) return MAX_RENDER_NOTES - 1;
    return idx;
}
