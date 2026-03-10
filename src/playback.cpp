/*
 * Chordshift - Playback
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
#include "extensions.h"
#include "render.h"
#include "transforms.h"

// ============================================================================
// STEP ADVANCEMENT
// ============================================================================

// Calculate next step based on play mode, skipping disabled steps.
// Returns 0-based step index, or -1 if no enabled steps.
static int calculateNextStep(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);
    int playMode = v[kParamPlayMode];

    // Clamp currentPlayStep to valid range in case stepCount was reduced
    int current = dtc->currentPlayStep;
    if (current >= stepCount) current = stepCount - 1;

    int nextStep = -1;
    int skipDir = 1;

    switch (playMode) {
        case PLAY_FORWARD:
            nextStep = (current + 1) % stepCount;
            skipDir = 1;
            break;

        case PLAY_REVERSE:
            nextStep = current - 1;
            if (nextStep < 0) nextStep = stepCount - 1;
            skipDir = -1;
            break;

        case PLAY_PENDULUM:
            if (stepCount <= 1) {
                nextStep = 0;
            } else {
                if (dtc->pendulumDir == 0) {
                    // Forward
                    nextStep = current + 1;
                    if (nextStep >= stepCount) {
                        nextStep = stepCount - 2;
                        dtc->pendulumDir = 1;
                    }
                } else {
                    // Backward
                    nextStep = current - 1;
                    if (nextStep < 0) {
                        nextStep = 1;
                        dtc->pendulumDir = 0;
                    }
                }
                skipDir = (dtc->pendulumDir == 0) ? 1 : -1;
            }
            break;

        case PLAY_RANDOM:
            nextStep = randRange(alg->randState, 0, stepCount - 1);
            skipDir = 1;
            break;
    }

    if (nextStep < 0) nextStep = 0;

    // Skip disabled steps in the current play direction
    for (int attempt = 0; attempt < stepCount; attempt++) {
        StepParams sp = StepParams::fromAlgorithm(v, nextStep);
        if (sp.enabled()) {
            return nextStep;
        }
        nextStep = (nextStep + skipDir + stepCount) % stepCount;
    }

    return -1;  // No enabled steps
}

// ============================================================================
// TRANSPORT
// ============================================================================

void handleTransportStart(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;

    dtc->currentPlayStep = 0;
    dtc->pendulumDir = 0;
    dtc->firstTick = true;
    dtc->clockDivCounter = 0;
    dtc->stepHoldCounter = 0;
    dtc->stepTime = 0.0f;
    // Preserve stepDuration from previous run so first chord gate is accurate
    dtc->msAccum = 0.0f;
    dtc->transportState = TRANSPORT_RUNNING;
}

void handleTransportStop(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;

    dtc->transportState = TRANSPORT_STOPPED;

    killAllPlayingNotes(alg);
    sendAllNotesOff(alg);

    dtc->currentPlayStep = 0;
    dtc->firstTick = false;
    dtc->clockDivCounter = 0;
    dtc->stepHoldCounter = 0;
    dtc->stepTime = 0.0f;
}

// ============================================================================
// CLOCK TICK PROCESSING
// ============================================================================

void processClockTick(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    // Hold: consume remaining hold ticks silently
    if (!dtc->firstTick && dtc->stepHoldCounter > 0) {
        dtc->stepHoldCounter--;
        return;
    }

    // Kill any currently playing notes before new step
    killAllPlayingNotes(alg);
    sendAllNotesOff(alg);

    // Calculate next step
    int nextStep;
    if (dtc->firstTick) {
        dtc->firstTick = false;
        // First tick: find first enabled step from step 0
        nextStep = -1;
        int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);
        for (int i = 0; i < stepCount; i++) {
            StepParams sp = StepParams::fromAlgorithm(v, i);
            if (sp.enabled()) {
                nextStep = i;
                break;
            }
        }
    } else {
        nextStep = calculateNextStep(alg);
    }

    if (nextStep < 0) return;  // No enabled steps

    dtc->currentPlayStep = (uint8_t)nextStep;

    // Check probability gate and read step params
    StepParams sp = StepParams::fromAlgorithm(v, nextStep);

    // Set hold counter for this step
    dtc->stepHoldCounter = (uint8_t)(sp.hold() - 1);
    int prob = sp.probability();
    if (prob < 100) {
        if ((int)(randFloat(alg->randState) * 100.0f) >= prob) {
            return;  // Step skipped by probability
        }
    }

    // Get base chord for this step (template or captured)
    StepState* ss = &alg->stepStates[nextStep];
    int tmpl = sp.chordTemplate();

    DegreeBuffer buf;
    if (tmpl > 0) {
        const ChordTemplate& t = CHORD_TEMPLATES[tmpl];
        buf.count = t.count;
        for (int i = 0; i < buf.count; i++)
            buf.degrees[i] = t.degrees[i];
    } else {
        if (ss->baseChord.count == 0) return;  // Empty step
        buf.count = ss->baseChord.count;
        for (int i = 0; i < buf.count; i++)
            buf.degrees[i] = ss->baseChord.degrees[i];
    }

    // Apply extensions (before transform pipeline)
    applyExtensions(&buf, v);

    // Apply transform pipeline
    applyTransforms(&buf, v, nextStep, alg->randState);

    // Render to MIDI (scale gate duration by hold value)
    RenderedChord rendered;
    float effectiveDuration = dtc->stepDuration * (float)sp.hold();
    renderChord(&rendered, &buf, v, nextStep, effectiveDuration, alg->randState);

    // Cache for UI display
    ss->lastRendered = rendered;

    // Emit MIDI
    emitRenderedChord(alg, &rendered);
}
