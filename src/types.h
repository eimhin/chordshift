/*
 * MIDI Chords - Types and Constants
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
static constexpr int DIR_PINGPONG = 2;
static constexpr int DIR_INSIDE_OUT = 3;
static constexpr int DIR_OUTSIDE_IN = 4;
static constexpr int DIR_RANDOM = 5;

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
    kParamPlayMode,
    kParamStepCount,
    kParamCaptureNorm,
    kParamClearStep,
    kParamClearAll,

    kGlobalParamCount  // = 28
};

// Per-step parameter offsets
enum {
    kStepEnabled = 0,
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

    kStepParamCount  // = 12
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
struct MidiChords_DTC {
    TransportState transportState;
    bool prevGateHigh;
    bool prevClockHigh;
    float stepTime;
    float stepDuration;
    float msAccum;         // Fractional millisecond accumulator for timing
    uint8_t currentPlayStep;   // 0-based index of current playing step
    uint8_t pendulumDir;       // 0=forward, 1=backward
    bool firstTick;

    // Parameter change detection
    int16_t lastRecord;
    int16_t lastClearStep;
    int16_t lastClearAll;

    // Capture state (lightweight counters only — buffers live in SRAM)
    uint8_t captureCount;
    uint8_t snapshotCount;
    uint8_t inputVel;
};

// UI layout constants
static constexpr int UI_LEFT_MARGIN = 2;
static constexpr int UI_BRIGHTNESS_MAX = 15;
static constexpr int UI_BRIGHTNESS_DIM = 1;
static constexpr int UI_BRIGHTNESS_MED = 6;

// Step grid layout
static constexpr int UI_STEP_Y = 10;
static constexpr int UI_STEP_HEIGHT = 18;
static constexpr int UI_STEP_WIDTH = 28;
static constexpr int UI_STEP_GAP = 2;

// Chord visualization
static constexpr int UI_CHORD_Y = 32;
static constexpr int UI_CHORD_HEIGHT = 28;

// Main algorithm structure (SRAM)
struct MidiChordsAlgorithm : public _NT_algorithm {
    MidiChords_DTC* dtc;
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

    // Active output note count (for early-exit optimization)
    uint8_t activeNoteCount;

    // PRNG state
    uint32_t randState;

    MidiChordsAlgorithm(MidiChords_DTC* dtc_, StepState* stepStates_)
        : dtc(dtc_), stepStates(stepStates_) {}
};
