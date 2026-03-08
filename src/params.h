/*
 * Chordshift - Parameter Definitions
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
static const char* const directionStrings[] = {"Up", "Down", "Pendulum", "PingPong", "Diverge", "Converge",
                                               "Random", "Pedal Lo", "Pedal Hi", NULL};
static const char* const velCurveStrings[] = {"Ramp", "Curve", "Peak", "Step", "Random", NULL};
static const char* const timeCurveStrings[] = {"Off", "Ramp", "Curve", "Peak", "Step", "Random", NULL};
static const char* const playModeStrings[] = {"Forward", "Reverse", "Pendulum", "Random", NULL};
static const char* const clockDivStrings[] = {"/1", "/2", "/3", "/4", "/6", "/8", "/12", "/16", NULL};
static const char* const templateStrings[] = {"Custom", "Note", "Fifth", "Triad", "7th",
                                              "Sus2", "Sus4", "Shell", "Quartal", "Cluster", NULL};
static const char* const contourStrings[] = {"Random", "Arc", "Rise", "Fall", NULL};
static const char* const seqLenStrings[] = {"None", "8", "16", "32", "64", "128", "256", NULL};
static const char* const seqDivStrings[] = {"None", "1", "2", "4", "8", "16", "32", NULL};
static const char* const seqHoldStrings[] = {"Varied", "Uniform", NULL};
static const char* const randTemplateStrings[] = {"Off", "5th", "Triad", "7th", "5th+Tri", "5th+7th", "Tri+7th", "All", NULL};
// ============================================================================
// STEP PARAMETER MACRO
// ============================================================================

// clang-format off
#define STEP_PARAMS \
    {.name = "Template", .min = 0, .max = 9, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = templateStrings}, \
    {.name = "Enabled", .min = 0, .max = 1, .def = 1, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings}, \
    {.name = "Transpose", .min = -14, .max = 14, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Inversion", .min = -4, .max = 4, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Rotation", .min = -7, .max = 7, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Spread", .min = -7, .max = 7, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Reverse", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings}, \
    {.name = "Strum", .min = 0, .max = 100, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Velocity", .min = -64, .max = 64, .def = 0, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Gate", .min = 10, .max = 200, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Prob", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Reflect", .min = 0, .max = 3, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = reflectStrings}, \
    {.name = "Repeat", .min = 1, .max = 4, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Hold", .min = 1, .max = 8, .def = 1, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Direction", .min = 0, .max = 8, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = directionStrings}, \
    {.name = "Oct Random", .min = -100, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Inv Random", .min = -100, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL}, \
    {.name = "Density", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
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
    {.name = "MIDI In Ch", .min = 0, .max = 16, .def = 1, .unit = kNT_unitHasStrings, .scaling = 0, .enumStrings = NULL},
    {.name = "MIDI Out Ch", .min = 1, .max = 16, .def = 2, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Destination", .min = 0, .max = 4, .def = 3, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = midiDestStrings},

    // Output (7)
    {.name = "Velocity", .min = 1, .max = 127, .def = 100, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},

    // Scale (8-10)
    {.name = "Root", .min = 0, .max = 11, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = scaleRootStrings},
    {.name = "Scale", .min = 0, .max = 10, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = scaleTypeStrings},
    {.name = "Octave", .min = 1, .max = 8, .def = 6, .unit = kNT_unitHasStrings, .scaling = 0, .enumStrings = NULL},

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

    // Articulation (20-23)
    {.name = "Strum", .min = 0, .max = 100, .def = 0, .unit = kNT_unitMs, .scaling = 0, .enumStrings = NULL},
    {.name = "Vel Shape", .min = 0, .max = 4, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = velCurveStrings},
    {.name = "Vel Depth", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Vel Deviation", .min = 0, .max = 100, .def = 5, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},

    // Time curve (24-25)
    {.name = "Time Shape", .min = 0, .max = 5, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = timeCurveStrings},
    {.name = "Time Depth", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},

    // Playback (26-28)
    {.name = "Play Mode", .min = 0, .max = 3, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = playModeStrings},
    {.name = "Steps", .min = 1, .max = 8, .def = 8, .unit = kNT_unitNone, .scaling = 0, .enumStrings = NULL},
    {.name = "Clock Div", .min = 0, .max = 7, .def = 3, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = clockDivStrings},

    // Capture (28)
    {.name = "Capture Norm", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = offOnStrings},

    // Triggers (29-32)
    {.name = "Clear Step", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},
    {.name = "Clear All", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},
    {.name = "Copy Step", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},
    {.name = "Paste Step", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},
    {.name = "Reset All", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},

    // Randomize (34-46)
    {.name = "Contour", .min = 0, .max = 3, .def = 1, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = contourStrings},
    {.name = "Randomize", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = noYesStrings},
    {.name = "Seq Length", .min = 0, .max = 6, .def = 3, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = seqLenStrings},
    {.name = "Seq Div", .min = 0, .max = 6, .def = 3, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = seqDivStrings},
    {.name = "Seq Hold", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = seqHoldStrings},
    {.name = "Template", .min = 0, .max = 7, .def = 6, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = randTemplateStrings},
    {.name = "Transpose %", .min = 0, .max = 100, .def = 40, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Inversion %", .min = 0, .max = 100, .def = 25, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Rotation %", .min = 0, .max = 100, .def = 10, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Spread %", .min = 0, .max = 100, .def = 10, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Reverse %", .min = 0, .max = 100, .def = 25, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Gate %", .min = 0, .max = 100, .def = 25, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},
    {.name = "Repeat %", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL},

    // Per-step parameters (47 + 15*8 = 167 total)
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

static_assert(MAX_TOTAL_PARAMS == ARRAY_SIZE(parameters),
              "MAX_TOTAL_PARAMS must match actual parameters array size");

// ============================================================================
// PARAMETER PAGES
// ============================================================================

// Page 0: Setup
static const uint8_t pageSetup[] = {kParamRunInput, kParamClockInput, kParamMidiInCh, kParamMidiOutCh, kParamDestination, kParamBaseVelocity};

// Page 1: Scale
static const uint8_t pageScale[] = {kParamScaleRoot, kParamScaleType, kParamOctaveBase};

// Page 2: Record
static const uint8_t pageRecord[] = {kParamRecord, kParamCurrentStep, kParamCaptureNorm, kParamClearStep, kParamClearAll, kParamCopyStep, kParamPasteStep, kParamResetAll};

// Page 3: Playback
static const uint8_t pagePlayback[] = {kParamPlayMode, kParamStepCount, kParamClockDiv};

// Page 4: Pitch
static const uint8_t pagePitch[] = {kParamTranspose, kParamReflectMode, kParamSpreadAmount, kParamSpreadAnchor};

// Page 5: Voicing
static const uint8_t pageVoicing[] = {kParamInversion, kParamRotation, kParamNormalize, kParamDirection, kParamReverse};

// Page 6: Articulate
static const uint8_t pageArticulate[] = {kParamStrumTime, kParamVelCurve, kParamVelDepth, kParamVelDeviation, kParamTimeCurve, kParamTimeDepth};

// Page 7: Randomize
static const uint8_t pageRandomize[] = {
    kParamRandomize, kParamRandomContour,
    kParamRandSeqLen, kParamRandSeqDiv, kParamRandSeqHold,
    kParamRandTemplate, kParamRandTranspose,
    kParamRandInversion, kParamRandRotation, kParamRandSpread,
    kParamRandReverse, kParamRandGate, kParamRandRepeat
};

// ============================================================================
// DYNAMIC PAGE BUILDING
// ============================================================================

struct GlobalPageInfo {
    const char* name;
    const uint8_t* params;
    uint8_t numParams;
};

static const GlobalPageInfo globalPages[] = {
    {"Setup",      pageSetup,      ARRAY_SIZE(pageSetup)},
    {"Scale",      pageScale,      ARRAY_SIZE(pageScale)},
    {"Record",     pageRecord,     ARRAY_SIZE(pageRecord)},
    {"Playback",   pagePlayback,   ARRAY_SIZE(pagePlayback)},
    {"Pitch",      pagePitch,      ARRAY_SIZE(pagePitch)},
    {"Voicing",    pageVoicing,    ARRAY_SIZE(pageVoicing)},
    {"Articulate", pageArticulate, ARRAY_SIZE(pageArticulate)},
    {"Randomize",  pageRandomize,  ARRAY_SIZE(pageRandomize)},
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
