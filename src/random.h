/*
 * MIDI Chords - Random Number Generation
 * SplitMix32 PRNG implementation (from midi-looper)
 */

#pragma once

#include <cstdint>

// ============================================================================
// PRNG (SplitMix32)
// ============================================================================

static inline uint32_t nextRand(uint32_t& state) {
    uint32_t z = (state += 0x9E3779B9u);
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

static inline int randRange(uint32_t& state, int min, int max) {
    if (min >= max) return min;
    return min + (int)(nextRand(state) % (uint32_t)(max - min + 1));
}

static inline float randFloat(uint32_t& state) {
    return (float)(nextRand(state) & 0xFFFFFF) / (float)0xFFFFFF;
}
