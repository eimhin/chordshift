/*
 * Chordshift - distingNT Plugin
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
#include "drift.h"
#include "math.h"
#include "midi.h"
#include "midi_utils.h"
#include "params.h"
#include "playback.h"
#include "randomize.h"
#include "scales.h"
#include "serial.h"
#include "types.h"
#include "ui.h"

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specs) {
    (void)specs;
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof(ChordshiftAlgorithm);
    req.dram = sizeof(StepState) * NUM_STEPS;
    req.dtc = sizeof(Chordshift_DTC);
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req,
                         const int32_t* specs) {
    (void)specs;
    Chordshift_DTC* dtc = (Chordshift_DTC*)ptrs.dtc;
    StepState* stepStates = (StepState*)ptrs.dram;

    // Initialize DTC (memset zeros all fields; only set non-zero values after)
    memset(dtc, 0, sizeof(Chordshift_DTC));
    dtc->stepDuration = 0.1f;
    dtc->focusedDriftStep = -1;

    // Initialize step states in DRAM
    for (int s = 0; s < NUM_STEPS; s++) {
        stepStates[s].baseChord.count = 0;
        stepStates[s].lastRendered.count = 0;
    }

    // Construct algorithm in SRAM
    ChordshiftAlgorithm* pThis = new (ptrs.sram) ChordshiftAlgorithm(dtc, stepStates);

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

    // Initialize active note counts
    pThis->activeNoteCount = 0;
    pThis->delayedNoteCount = 0;

    // Initialize clipboard
    pThis->clipboard.valid = false;

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
    ChordshiftAlgorithm* alg = (ChordshiftAlgorithm*)self;

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
    ChordshiftAlgorithm* alg = (ChordshiftAlgorithm*)self;
    Chordshift_DTC* dtc = alg->dtc;
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

    // Force trigger params off on first step() after construct/preset load.
    // Sync last* to suppress edge detection in case v[] hasn't updated yet.
    if (!dtc->initialized) {
        uint32_t idx = NT_algorithmIndex(self);
        uint32_t off = NT_parameterOffset();
        NT_setParameterFromAudio(idx, kParamClearStep + off, 0);
        NT_setParameterFromAudio(idx, kParamClearAll + off, 0);
        NT_setParameterFromAudio(idx, kParamCopyStep + off, 0);
        NT_setParameterFromAudio(idx, kParamPasteStep + off, 0);
        NT_setParameterFromAudio(idx, kParamRecord + off, 0);
        NT_setParameterFromAudio(idx, kParamResetAll + off, 0);
        NT_setParameterFromAudio(idx, kParamRandomize + off, 0);
        dtc->lastClearStep = v[kParamClearStep];
        dtc->lastClearAll = v[kParamClearAll];
        dtc->lastCopyStep = v[kParamCopyStep];
        dtc->lastPasteStep = v[kParamPasteStep];
        dtc->lastResetAll = v[kParamResetAll];
        dtc->lastRecord = v[kParamRecord];
        dtc->lastRandomize = v[kParamRandomize];
        dtc->initialized = true;
    }

    // Parameter change detection: Record toggle
    int record = v[kParamRecord];
    if (record == 1 && dtc->lastRecord == 0) {
        // Reset capture state on record arm to prevent ghost notes
        dtc->captureCount = 0;
        dtc->snapshotCount = 0;
        dtc->inputVel = 0;
        memset(alg->captureNotes, 0, sizeof(alg->captureNotes));
        memset(alg->snapshotNotes, 0, sizeof(alg->snapshotNotes));
    }
    dtc->lastRecord = (int16_t)record;

    // Parameter change detection: Clear Step
    int clearStep = v[kParamClearStep];
    if (clearStep == 1 && dtc->lastClearStep == 0) {
        int editStep = clamp(v[kParamCurrentStep] - 1, 0, NUM_STEPS - 1);
        alg->stepStates[editStep].baseChord.count = 0;
        alg->stepStates[editStep].lastRendered.count = 0;
    }
    dtc->lastClearStep = (int16_t)clearStep;

    // Parameter change detection: Clear All
    int clearAll = v[kParamClearAll];
    if (clearAll == 1 && dtc->lastClearAll == 0) {
        killAllPlayingNotes(alg);
        for (int s = 0; s < NUM_STEPS; s++) {
            alg->stepStates[s].baseChord.count = 0;
            alg->stepStates[s].lastRendered.count = 0;
        }
    }
    dtc->lastClearAll = (int16_t)clearAll;

    // Parameter change detection: Copy Step
    int copyStep = v[kParamCopyStep];
    if (copyStep == 1 && dtc->lastCopyStep == 0) {
        int editStep = clamp(v[kParamCurrentStep] - 1, 0, NUM_STEPS - 1);
        alg->clipboard.chord = alg->stepStates[editStep].baseChord;
        for (int i = 0; i < PARAMS_PER_STEP; i++) {
            alg->clipboard.params[i] = v[stepParam(editStep, i)];
        }
        alg->clipboard.valid = true;
    }
    dtc->lastCopyStep = (int16_t)copyStep;

    // Parameter change detection: Paste Step
    int pasteStep = v[kParamPasteStep];
    if (pasteStep == 1 && dtc->lastPasteStep == 0 && alg->clipboard.valid) {
        int editStep = clamp(v[kParamCurrentStep] - 1, 0, NUM_STEPS - 1);
        alg->stepStates[editStep].baseChord = alg->clipboard.chord;
        alg->stepStates[editStep].lastRendered.count = 0;
        uint32_t idx = NT_algorithmIndex(self);
        uint32_t off = NT_parameterOffset();
        for (int i = 0; i < PARAMS_PER_STEP; i++) {
            NT_setParameterFromAudio(idx, stepParam(editStep, i) + off, alg->clipboard.params[i]);
        }
    }
    dtc->lastPasteStep = (int16_t)pasteStep;

    // Parameter change detection: Reset All
    int resetAll = v[kParamResetAll];
    if (resetAll == 1 && dtc->lastResetAll == 0) {
        killAllPlayingNotes(alg);
        uint32_t idx = NT_algorithmIndex(self);
        uint32_t poff = NT_parameterOffset();
        for (int p = 0; p < MAX_TOTAL_PARAMS; p++) {
            NT_setParameterFromAudio(idx, p + poff, parameters[p].def);
        }
        for (int s = 0; s < NUM_STEPS; s++) {
            alg->stepStates[s].baseChord.count = 0;
            alg->stepStates[s].lastRendered.count = 0;
        }
        resetDrift(dtc);
    }
    dtc->lastResetAll = (int16_t)resetAll;

    // Parameter change detection: Randomize
    int randomize = v[kParamRandomize];
    if (randomize == 1 && dtc->lastRandomize == 0) {
        randomizeSequence(alg->randState, v,
                          NT_algorithmIndex(self), NT_parameterOffset(),
                          dtc->stepDuration, alg->stepStates);
    }
    dtc->lastRandomize = (int16_t)randomize;

    // Timing and delayed notes
    dtc->stepTime += dt;
    dtc->msAccum += dt * 1000.0f;
    int elapsedMs = (int)dtc->msAccum;
    dtc->msAccum -= (float)elapsedMs;
    processDelayedNotes(alg, elapsedMs);
    processNoteDurations(alg, elapsedMs);

    // Clock trigger processing (with division)
    if (clockRising && transportIsRunning(dtc->transportState)) {
        int divIdx = clamp(v[kParamClockDiv], 0, NUM_CLOCK_DIV_VALUES - 1);
        int division = CLOCK_DIV_VALUES[divIdx];

        bool shouldAdvance = false;

        if (dtc->firstTick) {
            shouldAdvance = true;
            dtc->clockDivCounter = 0;
        } else {
            dtc->clockDivCounter++;
            if (dtc->clockDivCounter >= division) {
                dtc->clockDivCounter = 0;
                shouldAdvance = true;
            }
        }

        if (shouldAdvance) {
            if (dtc->stepTime > 0.001f && !dtc->firstTick) {
                dtc->stepDuration = dtc->stepTime;
            }
            dtc->stepTime = 0.0f;
            processClockTick(alg);
        }
    }
}

// ============================================================================
// MIDI HANDLING
// ============================================================================

void midiMessage(_NT_algorithm* self, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    ChordshiftAlgorithm* alg = (ChordshiftAlgorithm*)self;
    Chordshift_DTC* dtc = alg->dtc;
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

int parameterString(_NT_algorithm* self, int p, int v, char* buff) {
    (void)self;
    if (p == kParamMidiInCh) {
        if (v == 0) {
            buff[0] = 'A'; buff[1] = 'l'; buff[2] = 'l'; buff[3] = '\0';
            return 3;
        }
        return NT_intToString(buff, v);
    }
    if (p == kParamOctaveBase) {
        buff[0] = 'C';
        int len = 1 + NT_intToString(buff + 1, v - 2);
        return len;
    }
    return 0;
}

bool draw(_NT_algorithm* self) {
    ChordshiftAlgorithm* alg = (ChordshiftAlgorithm*)self;
    alg->dtc->drawFrameCount++;
    return drawUI(alg);
}

void serialise(_NT_algorithm* self, _NT_jsonStream& stream) { serialiseData((ChordshiftAlgorithm*)self, stream); }

bool deserialise(_NT_algorithm* self, _NT_jsonParse& parse) {
    return deserialiseData((ChordshiftAlgorithm*)self, parse);
}

int parameterUiPrefix(_NT_algorithm* self, int p, char* buff) {
    (void)self;
    if (p < kGlobalParamCount)
        return 0;
    int stepIdx = (p - kGlobalParamCount) / PARAMS_PER_STEP;
    int len = NT_intToString(buff, stepIdx + 1);
    buff[len++] = ':';
    buff[len] = 0;
    return len;
}

// ============================================================================
// FACTORY DEFINITION
// ============================================================================

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('C', 'S', 'h', '1'), // Chordshift v1
    .name = "Chordshift",
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
    .tags = kNT_tagInstrument | kNT_tagUtility,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
    .serialise = serialise,
    .deserialise = deserialise,
    .midiSysEx = NULL,
    .parameterUiPrefix = parameterUiPrefix,
    .parameterString = parameterString,
};

// ============================================================================
// PLUGIN ENTRY POINT
// ============================================================================

uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)((data == 0) ? &factory : NULL);
    }
    return 0;
}
