/*
 * MIDI Chords - Playback
 *
 * Transport follows midi-looper pattern:
 * - Gate rising edge -> start (reset to step 0)
 * - Gate falling edge -> stop (all notes off)
 * - Clock rising edge -> advance step and render
 *
 * Play modes: Forward, Reverse, Pendulum, Random
 */

#include "playback.h"
#include "math.h"
#include "midi.h"
#include "random.h"
#include "render.h"
#include "transforms.h"

// ============================================================================
// STEP ADVANCEMENT
// ============================================================================

// Calculate next step based on play mode, skipping disabled steps.
// Returns 0-based step index, or -1 if no enabled steps.
static int calculateNextStep(MidiChordsAlgorithm* alg) {
    MidiChords_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);
    int playMode = v[kParamPlayMode];

    int nextStep = -1;

    switch (playMode) {
        case PLAY_FORWARD:
            nextStep = (dtc->currentPlayStep + 1) % stepCount;
            break;

        case PLAY_REVERSE:
            nextStep = dtc->currentPlayStep - 1;
            if (nextStep < 0) nextStep = stepCount - 1;
            break;

        case PLAY_PENDULUM:
            if (stepCount <= 1) {
                nextStep = 0;
            } else {
                if (dtc->pendulumDir == 0) {
                    // Forward
                    nextStep = dtc->currentPlayStep + 1;
                    if (nextStep >= stepCount) {
                        nextStep = stepCount - 2;
                        dtc->pendulumDir = 1;
                    }
                } else {
                    // Backward
                    nextStep = dtc->currentPlayStep - 1;
                    if (nextStep < 0) {
                        nextStep = 1;
                        dtc->pendulumDir = 0;
                    }
                }
            }
            break;

        case PLAY_RANDOM:
            nextStep = randRange(alg->randState, 0, stepCount - 1);
            break;
    }

    if (nextStep < 0) nextStep = 0;

    // Skip disabled steps (try up to stepCount times to find an enabled step)
    for (int attempt = 0; attempt < stepCount; attempt++) {
        StepParams sp = StepParams::fromAlgorithm(v, nextStep);
        if (sp.enabled()) {
            return nextStep;
        }
        // Advance to next step in forward direction to find an enabled one
        nextStep = (nextStep + 1) % stepCount;
    }

    return -1;  // No enabled steps
}

// ============================================================================
// TRANSPORT
// ============================================================================

void handleTransportStart(MidiChordsAlgorithm* alg) {
    MidiChords_DTC* dtc = alg->dtc;

    dtc->currentPlayStep = 0;
    dtc->pendulumDir = 0;
    dtc->clockCount = 0;
    dtc->stepTime = 0.0f;
    dtc->transportState = transportTransition_Start(dtc->transportState);
}

void handleTransportStop(MidiChordsAlgorithm* alg) {
    MidiChords_DTC* dtc = alg->dtc;

    dtc->transportState = transportTransition_Stop(dtc->transportState);

    killAllPlayingNotes(alg);
    sendAllNotesOff(alg);

    dtc->currentPlayStep = 0;
    dtc->clockCount = 0;
    dtc->stepTime = 0.0f;
}

// ============================================================================
// CLOCK TICK PROCESSING
// ============================================================================

void processClockTick(MidiChordsAlgorithm* alg) {
    MidiChords_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    dtc->clockCount++;

    // Kill any currently playing notes before new step
    killAllPlayingNotes(alg);

    // Calculate next step
    int nextStep;
    if (dtc->clockCount == 1) {
        // First tick: start from step 0 (or first enabled step)
        nextStep = 0;
        int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);
        // Find first enabled step
        for (int i = 0; i < stepCount; i++) {
            StepParams sp = StepParams::fromAlgorithm(v, nextStep);
            if (sp.enabled()) break;
            nextStep = (nextStep + 1) % stepCount;
        }
    } else {
        nextStep = calculateNextStep(alg);
    }

    if (nextStep < 0) return;  // No enabled steps

    dtc->currentPlayStep = (uint8_t)nextStep;

    // Check probability gate
    StepParams sp = StepParams::fromAlgorithm(v, nextStep);
    int prob = sp.probability();
    if (prob < 100) {
        if ((int)(randFloat(alg->randState) * 100.0f) >= prob) {
            return;  // Step skipped by probability
        }
    }

    // Get base chord for this step
    StepState* ss = &alg->stepStates[nextStep];
    if (ss->baseChord.count == 0) return;  // Empty step

    // Copy base chord to working buffer
    DegreeBuffer buf;
    buf.count = ss->baseChord.count;
    for (int i = 0; i < buf.count; i++) {
        buf.degrees[i] = ss->baseChord.degrees[i];
    }

    // Apply transform pipeline
    applyTransforms(&buf, v, nextStep, alg->randState);

    // Render to MIDI
    RenderedChord rendered;
    renderChord(&rendered, &buf, v, nextStep, dtc->stepDuration, alg->randState);

    // Cache for UI display
    ss->lastRendered = rendered;

    // Emit MIDI
    emitRenderedChord(alg, &rendered);
}
