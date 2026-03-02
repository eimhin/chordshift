/*
 * MIDI Chords - Parameter Definitions
 * Parameter arrays and page definitions for the distingNT UI
 */

#pragma once

#include "config.h"
#include "types.h"
#include <cstddef>

// ============================================================================
// PARAMETER STRING ARRAYS
// ============================================================================

static const char* const offOnStrings[] = {"Off", "On", NULL};
static const char* const noYesStrings[] = {"No", "Yes", NULL};
static const char* const midiDestStrings[] = {"Breakout", "SelectBus", "USB", "Internal", "All", NULL};
static const char* const scaleRootStrings[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B", NULL};
static const char* const scaleTypeStrings[] = {"Ionian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian",
                                               "Locrian", "Harm Min", "Melo Min", "Maj Penta", "Min Penta", NULL};
static const char* const reflectStrings[] = {"Off", "Root", "Lowest", "Highest", NULL};
static const char* const anchorStrings[] = {"Lowest", "Center", NULL};
static const char* const normalizeStrings[] = {"None", "Lowest=0", "First=0", NULL};
static const char* const directionStrings[] = {"Forward", "Backward", "Pendulum", "PingPong", "InsideOut", "OutsideIn",
                                               "Random", "BotRepeat", "TopRepeat", NULL};
static const char* const velCurveStrings[] = {"Linear", "Exp", "Triangle", "Square", "Random", NULL};
static const char* const timeCurveStrings[] = {"Off", "Linear", "Exp", "Triangle", "Square", "Random", NULL};
static const char* const playModeStrings[] = {"Forward", "Reverse", "Pendulum", "Random", NULL};
// ============================================================================
// STEP PARAMETER MACRO
// ============================================================================

// clang-format off
#define STEP_PARAMS \
    {.name = "Enabled", .min = 0, .max = 1, .def = 1, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings}, \
    {.name = "Transpose", .min = -14, .max = 14, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Inversion", .min = -4, .max = 4, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Rotation", .min = -7, .max = 7, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Spread", .min = -7, .max = 7, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Reverse", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings}, \
    {.name = "Strum", .min = 0, .max = 100, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Velocity", .min = -64, .max = 64, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Gate", .min = 1, .max = 200, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Prob", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Reflect", .min = 0, .max = 3, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = reflectStrings}, \
    {.name = "Repeat", .min = 1, .max = 4, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
// clang-format on

// ============================================================================
// PARAMETER DEFINITIONS
// ============================================================================

static const _NT_parameter parameters[MAX_TOTAL_PARAMS] = {
    // Routing parameters (0-1)
    NT_PARAMETER_CV_INPUT("Run", 0, 1)
    NT_PARAMETER_CV_INPUT("Clock", 0, 2)

    // Record/Edit (2-4)
    {.name = "Record", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = offOnStrings},
    {.name = "Edit Step", .min = 1, .max = 8, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},

    // MIDI (4-6)
    {.name = "MIDI In Ch", .min = 0, .max = 16, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "MIDI Out Ch", .min = 1, .max = 16, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Destination", .min = 0, .max = 4, .def = 3, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = midiDestStrings},

    // Output (7)
    {.name = "Velocity", .min = 1, .max = 127, .def = 100, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},

    // Scale (8-10)
    {.name = "Root", .min = 0, .max = 11, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = scaleRootStrings},
    {.name = "Scale", .min = 0, .max = 10, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = scaleTypeStrings},
    {.name = "Octave", .min = 1, .max = 7, .def = 3, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},

    // Pitch transforms (11-14)
    {.name = "Transpose", .min = -14, .max = 14, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Reflect", .min = 0, .max = 3, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = reflectStrings},
    {.name = "Spread", .min = -7, .max = 7, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Spread Anchor", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = anchorStrings},

    // Voicing (15-17)
    {.name = "Inversion", .min = -4, .max = 4, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Rotation", .min = -7, .max = 7, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Normalize", .min = 0, .max = 2, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = normalizeStrings},

    // Order (18-19)
    {.name = "Direction", .min = 0, .max = 8, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = directionStrings},
    {.name = "Reverse", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},

    // Articulation (20-22)
    {.name = "Strum", .min = 0, .max = 100, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL},
    {.name = "Vel Curve", .min = 0, .max = 4, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = velCurveStrings},
    {.name = "Vel Depth", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},

    // Time curve (23-24)
    {.name = "Time Curve", .min = 0, .max = 5, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = timeCurveStrings},
    {.name = "Time Depth", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},

    // Playback (25-26)
    {.name = "Play Mode", .min = 0, .max = 3, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = playModeStrings},
    {.name = "Steps", .min = 1, .max = 8, .def = 8, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},

    // Capture (27)
    {.name = "Capture Norm", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = offOnStrings},

    // Triggers (28-29)
    {.name = "Clear Step", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},
    {.name = "Clear All", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},

    // Per-step parameters (30 + 12*8 = 126 total)
    STEP_PARAMS  // Step 1
    STEP_PARAMS  // Step 2
    STEP_PARAMS  // Step 3
    STEP_PARAMS  // Step 4
    STEP_PARAMS  // Step 5
    STEP_PARAMS  // Step 6
    STEP_PARAMS  // Step 7
    STEP_PARAMS  // Step 8
};

#undef STEP_PARAMS

// ============================================================================
// PARAMETER PAGES
// ============================================================================

// Page 0: Routing
static const uint8_t pageRouting[] = {kParamRunInput, kParamClockInput};

// Page 1: Scale
static const uint8_t pageScale[] = {kParamScaleRoot, kParamScaleType, kParamOctaveBase};

// Page 2: Record
static const uint8_t pageRecord[] = {kParamRecord, kParamCurrentStep, kParamMidiInCh, kParamCaptureNorm, kParamClearStep, kParamClearAll};

// Page 3: Output
static const uint8_t pageOutput[] = {kParamMidiOutCh, kParamDestination, kParamBaseVelocity};

// Page 4: Pitch
static const uint8_t pagePitch[] = {kParamTranspose, kParamReflectMode, kParamSpreadAmount, kParamSpreadAnchor};

// Page 5: Voicing
static const uint8_t pageVoicing[] = {kParamInversion, kParamRotation, kParamNormalize};

// Page 6: Order
static const uint8_t pageOrder[] = {kParamDirection, kParamReverse, kParamPlayMode, kParamStepCount};

// Page 7: Articulate
static const uint8_t pageArticulate[] = {kParamStrumTime, kParamVelCurve, kParamVelDepth, kParamTimeCurve, kParamTimeDepth};

// ============================================================================
// DYNAMIC PAGE BUILDING
// ============================================================================

struct GlobalPageInfo {
    const char* name;
    const uint8_t* params;
    uint8_t numParams;
};

static const GlobalPageInfo globalPages[] = {
    {"Routing",    pageRouting,    sizeof(pageRouting) / sizeof(pageRouting[0])},
    {"Scale",      pageScale,      sizeof(pageScale) / sizeof(pageScale[0])},
    {"Record",     pageRecord,     sizeof(pageRecord) / sizeof(pageRecord[0])},
    {"Output",     pageOutput,     sizeof(pageOutput) / sizeof(pageOutput[0])},
    {"Pitch",      pagePitch,      sizeof(pagePitch) / sizeof(pagePitch[0])},
    {"Voicing",    pageVoicing,    sizeof(pageVoicing) / sizeof(pageVoicing[0])},
    {"Order",      pageOrder,      sizeof(pageOrder) / sizeof(pageOrder[0])},
    {"Articulate", pageArticulate, sizeof(pageArticulate) / sizeof(pageArticulate[0])},
};

static constexpr int NUM_GLOBAL_PAGES = 8;
static_assert(sizeof(globalPages) / sizeof(globalPages[0]) == NUM_GLOBAL_PAGES,
              "globalPages[] size must match NUM_GLOBAL_PAGES");

static const char* const stepPageNames[] = {"Step 1", "Step 2", "Step 3", "Step 4",
                                            "Step 5", "Step 6", "Step 7", "Step 8"};

static inline void buildStepPageIndices(uint8_t* indices, int step) {
    int base = kGlobalParamCount + (step * PARAMS_PER_STEP);
    for (int i = 0; i < PARAMS_PER_STEP; i++) {
        indices[i] = (uint8_t)(base + i);
    }
}
