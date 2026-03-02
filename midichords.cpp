/*
 * MIDI Chords - distingNT Plugin
 *
 * Diatonic Transform Chord Sequencer: An 8-step chord sequencer where each
 * step stores a base chord as scale-degree offsets and applies a non-destructive
 * transform pipeline at playback. All pitch logic operates in degree space;
 * MIDI notes are generated only at render time.
 *
 * Inputs:
 * - In1: Run gate (rising edge starts; falling edge stops)
 * - In2: Clock trigger (advances step position)
 *
 * Copyright (c) 2025 Eimhin Rooney
 */

#include <cstring>
#include <new>

// Module headers
#include "capture.h"
#include "math.h"
#include "midi.h"
#include "midi_utils.h"
#include "params.h"
#include "playback.h"
#include "scales.h"
#include "serial.h"
#include "types.h"
#include "ui.h"

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specs) {
    (void)specs;
    req.numParameters = MAX_TOTAL_PARAMS;
    req.sram = sizeof(MidiChordsAlgorithm);
    req.dram = sizeof(StepState) * NUM_STEPS;
    req.dtc = sizeof(MidiChords_DTC);
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req,
                         const int32_t* specs) {
    (void)specs;
    MidiChords_DTC* dtc = (MidiChords_DTC*)ptrs.dtc;
    StepState* stepStates = (StepState*)ptrs.dram;

    // Initialize DTC (memset zeros all fields; only set non-zero values after)
    memset(dtc, 0, sizeof(MidiChords_DTC));
    dtc->stepDuration = 0.1f;

    // Initialize step states in DRAM
    for (int s = 0; s < NUM_STEPS; s++) {
        stepStates[s].baseChord.count = 0;
        stepStates[s].lastRendered.count = 0;
    }

    // Construct algorithm in SRAM
    MidiChordsAlgorithm* pThis = new (ptrs.sram) MidiChordsAlgorithm(dtc, stepStates);

    // Initialize playing notes and capture buffers
    for (int i = 0; i < 128; i++) {
        pThis->playing[i].active = false;
        pThis->captureNotes[i] = 0;
        pThis->snapshotNotes[i] = 0;
        pThis->noteMap[i] = (uint8_t)i;
    }

    // Initialize delayed notes
    for (int i = 0; i < MAX_DELAYED_NOTES; i++) {
        pThis->delayedNotes[i].active = false;
    }

    // Initialize active note count
    pThis->activeNoteCount = 0;

    // Seed PRNG
    pThis->randState = NT_getCpuCycleCount();

    // Build parameter pages: global pages from table
    for (int i = 0; i < NUM_GLOBAL_PAGES; i++) {
        pThis->pageDefs[i] = {
            .name = globalPages[i].name, .numParams = globalPages[i].numParams,
            .group = (uint8_t)i, .unused = {0, 0}, .params = globalPages[i].params};
    }

    // Step pages
    for (int s = 0; s < NUM_STEPS; s++) {
        buildStepPageIndices(pThis->pageStepIndices[s], s);
        pThis->pageDefs[NUM_GLOBAL_PAGES + s] = {
            .name = stepPageNames[s],
            .numParams = PARAMS_PER_STEP,
            .group = (uint8_t)NUM_GLOBAL_PAGES,
            .unused = {0, 0},
            .params = pThis->pageStepIndices[s]};
    }

    pThis->dynamicPages.numPages = NUM_GLOBAL_PAGES + NUM_STEPS;
    pThis->dynamicPages.pages = pThis->pageDefs;

    // Copy parameter definitions into mutable array
    memcpy(pThis->paramDefs, parameters, sizeof(_NT_parameter) * MAX_TOTAL_PARAMS);

    // Set up parameters and pages
    pThis->parameters = pThis->paramDefs;
    pThis->parameterPages = &pThis->dynamicPages;

    (void)req;

    return pThis;
}

void parameterChanged(_NT_algorithm* self, int p) {
    MidiChordsAlgorithm* alg = (MidiChordsAlgorithm*)self;

    if (p == kParamStepCount) {
        int stepCount = clamp(alg->v[kParamStepCount], 1, NUM_STEPS);

        alg->dynamicPages.numPages = NUM_GLOBAL_PAGES + stepCount;
        NT_updateParameterPages(NT_algorithmIndex(self));

        alg->paramDefs[kParamCurrentStep].max = (int16_t)stepCount;
        NT_updateParameterDefinition(NT_algorithmIndex(self), kParamCurrentStep);
    }
}

// ============================================================================
// STEP FUNCTION (Audio rate processing)
// ============================================================================

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    MidiChordsAlgorithm* alg = (MidiChordsAlgorithm*)self;
    MidiChords_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    int numFrames = numFramesBy4 * 4;
    float dt = (float)numFrames / (float)NT_globals.sampleRate;

    // Read CV inputs
    int runBus = v[kParamRunInput];
    int clkBus = v[kParamClockInput];
    float gateVal = (runBus > 0) ? busFrames[(runBus - 1) * numFrames + numFrames - 1] : 0.0f;
    float clockVal = (clkBus > 0) ? busFrames[(clkBus - 1) * numFrames + numFrames - 1] : 0.0f;

    bool gateHigh = gateVal > GATE_THRESHOLD_HIGH;
    bool gateLow = gateVal < GATE_THRESHOLD_LOW;
    bool clockHigh = clockVal > GATE_THRESHOLD_HIGH;
    bool clockLow = clockVal < GATE_THRESHOLD_LOW;

    // Gate edge detection (transport control)
    if (gateHigh && !dtc->prevGateHigh) {
        handleTransportStart(alg);
    } else if (gateLow && dtc->prevGateHigh) {
        handleTransportStop(alg);
    }
    if (gateHigh) dtc->prevGateHigh = true;
    else if (gateLow) dtc->prevGateHigh = false;

    // Clock edge detection
    bool clockRising = clockHigh && !dtc->prevClockHigh;
    if (clockHigh) dtc->prevClockHigh = true;
    else if (clockLow) dtc->prevClockHigh = false;

    // Parameter change detection: Record toggle
    int record = v[kParamRecord];
    if (record != dtc->lastRecord) {
        // Reset capture state on any toggle to prevent ghost notes
        dtc->captureCount = 0;
        dtc->snapshotCount = 0;
        dtc->inputVel = 0;
        memset(alg->captureNotes, 0, sizeof(alg->captureNotes));
        memset(alg->snapshotNotes, 0, sizeof(alg->snapshotNotes));
        dtc->lastRecord = (int16_t)record;
    }

    // Parameter change detection: Clear Step
    int clearStep = v[kParamClearStep];
    if (clearStep != dtc->lastClearStep) {
        if (clearStep == 1) {
            int editStep = clamp(v[kParamCurrentStep] - 1, 0, NUM_STEPS - 1);
            alg->stepStates[editStep].baseChord.count = 0;
            alg->stepStates[editStep].lastRendered.count = 0;
        }
        dtc->lastClearStep = (int16_t)clearStep;
    }

    // Parameter change detection: Clear All
    int clearAll = v[kParamClearAll];
    if (clearAll != dtc->lastClearAll) {
        if (clearAll == 1) {
            killAllPlayingNotes(alg);
            for (int s = 0; s < NUM_STEPS; s++) {
                alg->stepStates[s].baseChord.count = 0;
                alg->stepStates[s].lastRendered.count = 0;
            }
        }
        dtc->lastClearAll = (int16_t)clearAll;
    }

    // Timing and delayed notes
    dtc->stepTime += dt;
    dtc->msAccum += dt * 1000.0f;
    int elapsedMs = (int)dtc->msAccum;
    dtc->msAccum -= (float)elapsedMs;
    processDelayedNotes(alg, elapsedMs);
    processNoteDurations(alg, elapsedMs);

    // Clock trigger processing
    if (clockRising && transportIsRunning(dtc->transportState)) {
        // Update step duration estimate (skip first tick — not a real inter-clock interval)
        if (dtc->stepTime > 0.001f && !dtc->firstTick) {
            dtc->stepDuration = dtc->stepTime;
        }
        dtc->stepTime = 0.0f;

        processClockTick(alg);
    }
}

// ============================================================================
// MIDI HANDLING
// ============================================================================

void midiMessage(_NT_algorithm* self, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    MidiChordsAlgorithm* alg = (MidiChordsAlgorithm*)self;
    MidiChords_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    uint8_t status = byte0 & 0xF0;
    uint8_t channel = byte0 & 0x0F;

    // Channel filter
    int channelFilter = v[kParamMidiInCh];
    if (channelFilter > 0 && channel != (channelFilter - 1)) {
        return;
    }

    int outCh = v[kParamMidiOutCh];
    uint32_t where = destToWhere(v[kParamDestination]);

    bool isNoteOn = (status == kMidiNoteOn && byte2 > 0);
    bool isNoteOff = (status == kMidiNoteOff || (status == kMidiNoteOn && byte2 == 0));

    // Recording mode check
    bool recording = (v[kParamRecord] == 1);

    // Scale quantization (white-key remap)
    if (isNoteOn) {
        uint8_t quantized = quantizeToScale(byte1, v[kParamScaleRoot], v[kParamScaleType]);
        alg->noteMap[byte1] = quantized;
        byte1 = quantized;
    } else if (isNoteOff) {
        byte1 = alg->noteMap[byte1];
    }

    // Pass-through: send remapped note to output so user hears scale notes
    if (isNoteOn || isNoteOff) {
        NT_sendMidi3ByteMessage(where, withChannel(status, outCh), byte1, byte2);
    }

    // Capture pipeline (only when recording)
    if (recording) {
        if (isNoteOn) {
            captureNoteOn(alg, byte1, byte2);
        } else if (isNoteOff) {
            captureNoteOff(alg, byte1);
        }
    } else {
        // Not recording — track held notes for velocity display
        if (isNoteOn) {
            trackHeldNote(alg, byte1, byte2);
        } else if (isNoteOff) {
            untrackHeldNote(alg, byte1);
            if (dtc->captureCount == 0) dtc->inputVel = 0;
        }
    }
}

// ============================================================================
// UI AND SERIALIZATION WRAPPERS
// ============================================================================

bool draw(_NT_algorithm* self) {
    MidiChordsAlgorithm* alg = (MidiChordsAlgorithm*)self;
    alg->dtc->drawFrameCount++;
    return drawUI(alg);
}

void serialise(_NT_algorithm* self, _NT_jsonStream& stream) { serialiseData((MidiChordsAlgorithm*)self, stream); }

bool deserialise(_NT_algorithm* self, _NT_jsonParse& parse) {
    return deserialiseData((MidiChordsAlgorithm*)self, parse);
}

// ============================================================================
// FACTORY DEFINITION
// ============================================================================

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('M', 'C', 'h', '1'), // MIDI Chords v1
    .name = "MIDI Chords",
    .description = "8-step diatonic transform chord sequencer",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = NULL,
    .midiMessage = midiMessage,
    .tags = kNT_tagUtility,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
    .serialise = serialise,
    .deserialise = deserialise,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = NULL,
};

// ============================================================================
// PLUGIN ENTRY POINT
// ============================================================================

uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version:
        return kNT_apiVersion13;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)((data == 0) ? &factory : NULL);
    }
    return 0;
}
