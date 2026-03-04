/*
 * Chordshift - Types and Constants
 * Data structures, enums, and state definitions for the chord sequencer
 */

#pragma once

#include <distingnt/api.h>
#include <cstdint>
#include "config.h"
#include "math.h"

// ============================================================================
// MIDI CONSTANTS
// ============================================================================

static constexpr uint8_t kMidiNoteOff = 0x80;
static constexpr uint8_t kMidiNoteOn = 0x90;
static constexpr uint8_t kMidiCC = 0xB0;

// ============================================================================
// STATE MACHINES
// ============================================================================

enum TransportState {
    TRANSPORT_STOPPED = 0,
    TRANSPORT_RUNNING = 1
};

static inline bool transportIsRunning(TransportState state) {
    return state != TRANSPORT_STOPPED;
}

// ============================================================================
// ENUM CONSTANTS
// ============================================================================

// Reflect modes
static constexpr int REFLECT_OFF = 0;
static constexpr int REFLECT_ROOT = 1;
static constexpr int REFLECT_LOWEST = 2;
static constexpr int REFLECT_HIGHEST = 3;

// Spread anchor
static constexpr int ANCHOR_LOWEST = 0;
static constexpr int ANCHOR_CENTER = 1;

// Normalize modes
static constexpr int NORM_NONE = 0;
static constexpr int NORM_LOWEST_TO_0 = 1;
static constexpr int NORM_FIRST_TO_0 = 2;

// Direction modes (note order within chord)
static constexpr int DIR_FORWARD = 0;
static constexpr int DIR_BACKWARD = 1;
static constexpr int DIR_PENDULUM = 2;
static constexpr int DIR_PINGPONG = 3;
static constexpr int DIR_INSIDE_OUT = 4;
static constexpr int DIR_OUTSIDE_IN = 5;
static constexpr int DIR_RANDOM = 6;
static constexpr int DIR_BOTTOM_REPEAT = 7;
static constexpr int DIR_TOP_REPEAT = 8;

// Velocity curve types
static constexpr int VCURVE_LINEAR = 0;
static constexpr int VCURVE_EXP = 1;
static constexpr int VCURVE_TRIANGLE = 2;
static constexpr int VCURVE_SQUARE = 3;
static constexpr int VCURVE_RANDOM = 4;

// Play modes (step sequencer direction)
static constexpr int PLAY_FORWARD = 0;
static constexpr int PLAY_REVERSE = 1;
static constexpr int PLAY_PENDULUM = 2;
static constexpr int PLAY_RANDOM = 3;

// Clock division lookup table
static constexpr int CLOCK_DIV_VALUES[] = {1, 2, 3, 4, 6, 8, 12, 16};
static constexpr int NUM_CLOCK_DIV_VALUES = 8;

// Gate detection thresholds
static constexpr float GATE_THRESHOLD_HIGH = 2.0f;
static constexpr float GATE_THRESHOLD_LOW = 0.5f;

// ============================================================================
// PARAMETER ENUMS
// ============================================================================

// Global parameter indices
enum {
    kParamRunInput = 0,
    kParamClockInput,
    kParamRecord,
    kParamCurrentStep,
    kParamMidiInCh,
    kParamMidiOutCh,
    kParamDestination,
    kParamBaseVelocity,
    kParamScaleRoot,
    kParamScaleType,
    kParamOctaveBase,
    kParamTranspose,
    kParamReflectMode,
    kParamSpreadAmount,
    kParamSpreadAnchor,
    kParamInversion,
    kParamRotation,
    kParamNormalize,
    kParamDirection,
    kParamReverse,
    kParamStrumTime,
    kParamVelCurve,
    kParamVelDepth,
    kParamTimeCurve,
    kParamTimeDepth,
    kParamPlayMode,
    kParamStepCount,
    kParamClockDiv,
    kParamCaptureNorm,
    kParamClearStep,
    kParamClearAll,
    kParamCopyStep,
    kParamPasteStep,

    kGlobalParamCount  // = 33
};

// Per-step parameter offsets
enum {
    kStepTemplate = 0,
    kStepEnabled,
    kStepTranspose,
    kStepInversion,
    kStepRotation,
    kStepSpread,
    kStepReverse,
    kStepStrumTime,
    kStepVelocity,
    kStepGateLength,
    kStepProbability,
    kStepReflect,
    kStepRepeat,
    kStepHold,
    kStepDirection,

    kStepParamCount  // = 15
};

// Validate parameter layout matches config constants
static_assert(PARAMS_PER_STEP == kStepParamCount,
              "PARAMS_PER_STEP must match kStepParamCount enum");
static_assert(GLOBAL_PARAMS == kGlobalParamCount,
              "GLOBAL_PARAMS must match kGlobalParamCount enum");

// Helper to get step parameter index
static inline int stepParam(int step, int param) {
    return kGlobalParamCount + (step * PARAMS_PER_STEP) + param;
}

// ============================================================================
// STEP PARAMETERS ACCESSOR
// ============================================================================

struct StepParams {
    const int16_t* v;
    int step;

    static StepParams fromAlgorithm(const int16_t* params, int stepIdx) {
        StepParams p;
        p.v = params;
        p.step = stepIdx;
        return p;
    }

    int raw(int param) const {
        return v[stepParam(step, param)];
    }

    int chordTemplate() const { return raw(kStepTemplate); }
    bool enabled() const { return raw(kStepEnabled) == 1; }
    int transpose() const { return raw(kStepTranspose); }
    int inversion() const { return raw(kStepInversion); }
    int rotation() const { return raw(kStepRotation); }
    int spread() const { return raw(kStepSpread); }
    bool reverse() const { return raw(kStepReverse) == 1; }
    int strumTime() const { return raw(kStepStrumTime); }
    int velocity() const { return raw(kStepVelocity); }
    int gateLength() const { return clamp(raw(kStepGateLength), 1, 200); }
    int probability() const { return clamp(raw(kStepProbability), 0, 100); }
    int reflect() const { return raw(kStepReflect); }
    int repeat() const { return clamp(raw(kStepRepeat), 1, 4); }
    int hold() const { return clamp(raw(kStepHold), 1, 8); }
    int direction() const { return raw(kStepDirection); }
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Stored per step — base chord as scale degree offsets
struct ChordDegrees {
    int8_t degrees[MAX_CHORD_NOTES];
    uint8_t count;
};

// Working buffer for transform pipeline
struct DegreeBuffer {
    int16_t degrees[MAX_RENDER_NOTES];
    uint8_t count;
};

// After degree->MIDI conversion + articulation
struct RenderedNote {
    uint8_t midiNote;
    uint8_t velocity;
    uint16_t delayMs;   // Strum offset
    uint16_t gateMs;
};

struct RenderedChord {
    RenderedNote notes[MAX_RENDER_NOTES];
    uint8_t count;
};

// Chord template definitions (degree-space shapes)
struct ChordTemplate {
    int8_t degrees[MAX_CHORD_NOTES];
    uint8_t count;
};

static const ChordTemplate CHORD_TEMPLATES[] = {
    {{},                    0}, // Custom (unused, placeholder)
    {{0},                   1}, // Note
    {{0, 4},                2}, // Fifth
    {{0, 2, 4},             3}, // Triad
    {{0, 2, 4, 6},          4}, // 7th
    {{0, 1, 4},             3}, // Sus2
    {{0, 3, 4},             3}, // Sus4
    {{0, 2, 6},             3}, // Shell
    {{0, 3, 6},             3}, // Quartal
    {{0, 1, 2},             3}, // Cluster
};

// Per-step runtime state (DRAM)
struct StepState {
    ChordDegrees baseChord;
    RenderedChord lastRendered;  // Cache for UI display
};

// Active note tracking
struct PlayingNote {
    uint32_t where;
    uint16_t remainingMs;
    uint8_t outCh;
    bool active;
};

// Delayed note for strum
struct DelayedNote {
    uint32_t where;
    uint16_t gateMs;
    uint16_t delayMs;
    uint8_t note;
    uint8_t velocity;
    uint8_t outCh;
    bool active;
};

// DTC — Fast-access global state
struct Chordshift_DTC {
    TransportState transportState;
    bool prevGateHigh;
    bool prevClockHigh;
    float stepTime;
    float stepDuration;
    float msAccum;         // Fractional millisecond accumulator for timing
    uint8_t currentPlayStep;   // 0-based index of current playing step
    uint8_t pendulumDir;       // 0=forward, 1=backward
    bool firstTick;
    uint8_t clockDivCounter;
    uint8_t stepHoldCounter;

    // Parameter change detection
    int16_t lastRecord;
    int16_t lastClearStep;
    int16_t lastClearAll;
    int16_t lastCopyStep;
    int16_t lastPasteStep;

    // Capture state (lightweight counters only — buffers live in SRAM)
    uint8_t captureCount;
    uint8_t snapshotCount;
    uint8_t inputVel;

    // UI frame counter for blink effects
    uint16_t drawFrameCount;

    // Set on first step() call to sync edge detection state after preset load
    bool initialized;
};

// UI layout constants
static constexpr int UI_LEFT_MARGIN = 2;
static constexpr int UI_BRIGHTNESS_MAX = 15;
static constexpr int UI_BRIGHTNESS_DIM = 1;
static constexpr int UI_BRIGHTNESS_LOW = 3;
static constexpr int UI_BRIGHTNESS_MED = 6;

// Blink timing
static constexpr int UI_BLINK_HALF_PERIOD = 15;

// Step grid layout
static constexpr int UI_STEP_Y = 10;
static constexpr int UI_STEP_HEIGHT = 20;
static constexpr int UI_STEP_WIDTH = 28;
static constexpr int UI_STEP_GAP = 2;
static constexpr int UI_FONT_NORMAL_ASCENT = 9;  // kNT_textNormal baseline to top

// Chord visualization
static constexpr int UI_CHORD_Y = 34;
static constexpr int UI_BAR_MAX_HEIGHT = 19;

// Step clipboard for copy/paste
struct StepClipboard {
    ChordDegrees chord;
    int16_t params[PARAMS_PER_STEP];
    bool valid;
};

// Main algorithm structure (SRAM)
struct ChordshiftAlgorithm : public _NT_algorithm {
    Chordshift_DTC* dtc;
    StepState* stepStates;       // -> DRAM
    PlayingNote playing[128];
    DelayedNote delayedNotes[MAX_DELAYED_NOTES];

    // Capture buffers (moved from DTC — too large for tightly-coupled memory)
    uint8_t captureNotes[128];   // Currently held (remapped) MIDI notes
    uint8_t snapshotNotes[128];  // Snapshot at peak chord size
    uint8_t noteMap[128];        // White-key remap tracking (original -> remapped)

    // Mutable copy of parameter definitions
    _NT_parameter paramDefs[MAX_TOTAL_PARAMS];

    // Dynamic parameter pages
    uint8_t pageStepIndices[NUM_STEPS][PARAMS_PER_STEP];
    _NT_parameterPage pageDefs[MAX_PAGES];
    _NT_parameterPages dynamicPages;

    // Clipboard for step copy/paste
    StepClipboard clipboard;

    // Active note counts (for early-exit optimization)
    uint8_t activeNoteCount;
    uint8_t delayedNoteCount;

    // PRNG state
    uint32_t randState;

    ChordshiftAlgorithm(Chordshift_DTC* dtc_, StepState* stepStates_)
        : dtc(dtc_), stepStates(stepStates_) {}
};
