/*
 * Chordshift - Math Utilities
 * Basic math functions
 */

#pragma once

// ============================================================================
// MATH UTILITIES
// ============================================================================

static inline int clamp(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}
