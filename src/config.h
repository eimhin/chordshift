/*
 * Chordshift - Configuration
 * Tunable constants for the diatonic transform chord sequencer
 */

#pragma once

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

// #define CHORDSHIFT_DEBUG

#ifdef CHORDSHIFT_DEBUG
#define DEBUG_LOG(fmt, ...) NT_logFormat(fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) ((void)0)
#endif

// ============================================================================
// SEQUENCER CONFIGURATION
// ============================================================================

static constexpr int NUM_STEPS = 8;            // Fixed 8-step sequencer
static constexpr int MAX_CHORD_NOTES = 8;      // Max notes per stored chord
static constexpr int MAX_RENDER_NOTES = 64;    // Max notes after transforms (direction + ratchet can expand)
static constexpr int MAX_DELAYED_NOTES = 64;   // Strum delay buffer size

// ============================================================================
// PARAMETER LAYOUT
// ============================================================================

static constexpr int PARAMS_PER_STEP = 18;     // Parameters per step
static constexpr int GLOBAL_PARAMS = 61;       // Global parameters

// Derived constants
static constexpr int MAX_TOTAL_PARAMS = GLOBAL_PARAMS + (PARAMS_PER_STEP * NUM_STEPS);
static constexpr int MAX_PAGES = 10 + NUM_STEPS; // 10 global pages + step pages

// ============================================================================
// COMPILE-TIME VALIDATION
// ============================================================================

// Color drift: maps offset to extension color threshold (50 + offset * DRIFT_COLOR_SCALE)
static constexpr int DRIFT_COLOR_SCALE = 10;

static_assert(MAX_TOTAL_PARAMS <= 242, "Max parameter index exceeds distingNT API limit of 242");
static_assert(MAX_CHORD_NOTES <= 127, "MAX_CHORD_NOTES must fit in int8_t array");
static_assert(NUM_STEPS == 8, "Changing NUM_STEPS requires updating: "
    "STEP_PARAMS repetitions in params.h, stepPageNames[] in params.h, "
    "and Edit Step max value in params.h");
