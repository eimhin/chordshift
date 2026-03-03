/*
 * Chordshift - Capture Pipeline
 *
 * When Record=On, incoming MIDI notes are accumulated. On each note-on,
 * snapshot the held notes (capturing latest chord at peak size). When last note released,
 * run the capture pipeline on the snapshot:
 *
 * 1. Convert to degree (midiNoteToDegree on already-quantized notes)
 * 2. Sort (insertion sort, N <= 8)
 * 3. Deduplicate
 * 4. Normalize (if CaptureNorm=On, subtract lowest so lowest=0)
 *
 * Steps 1-2 of the spec (white-key remap + pass-through) happen in
 * midiMessage() before calling these functions.
 */

#include "capture.h"
#include "math.h"
#include "scales.h"
#include <cstring>

// ============================================================================
// CAPTURE NOTE ON
// ============================================================================

void captureNoteOn(ChordshiftAlgorithm* alg, uint8_t note, uint8_t velocity) {
    Chordshift_DTC* dtc = alg->dtc;

    // Track held notes (guard against two keys quantizing to same pitch)
    trackHeldNote(alg, note, velocity);

    // Snapshot current held notes if this is a new peak
    if (dtc->captureCount >= dtc->snapshotCount) {
        memcpy(alg->snapshotNotes, alg->captureNotes, sizeof(alg->captureNotes));
        dtc->snapshotCount = dtc->captureCount;
    }
}

// ============================================================================
// CAPTURE NOTE OFF — COMMIT CHORD WHEN LAST NOTE RELEASED
// ============================================================================

void captureNoteOff(ChordshiftAlgorithm* alg, uint8_t note) {
    Chordshift_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    untrackHeldNote(alg, note);

    // Check if any notes still held
    if (dtc->captureCount > 0) return;

    // All notes released — commit the snapshot chord
    dtc->inputVel = 0;

    // Nothing captured
    if (dtc->snapshotCount == 0) return;

    int root = v[kParamScaleRoot];
    int scaleType = v[kParamScaleType];
    bool captureNorm = (v[kParamCaptureNorm] == 1);

    // Step 1: Convert snapshot MIDI notes to degrees
    int16_t degrees[MAX_CHORD_NOTES];
    int count = 0;

    for (int n = 0; n < 128 && count < MAX_CHORD_NOTES; n++) {
        if (alg->snapshotNotes[n]) {
            degrees[count++] = (int16_t)midiNoteToDegree((uint8_t)n, root, scaleType);
        }
    }

    // Step 2: Insertion sort (N <= 8)
    for (int i = 1; i < count; i++) {
        int16_t key = degrees[i];
        int j = i - 1;
        while (j >= 0 && degrees[j] > key) {
            degrees[j + 1] = degrees[j];
            j--;
        }
        degrees[j + 1] = key;
    }

    // Step 3: Deduplicate adjacent
    int unique = 0;
    for (int i = 0; i < count; i++) {
        if (i == 0 || degrees[i] != degrees[i - 1]) {
            degrees[unique++] = degrees[i];
        }
    }
    count = unique;

    // Step 4: Normalize (if CaptureNorm=On, subtract lowest so lowest=0)
    if (captureNorm && count > 0) {
        int16_t lowest = degrees[0];
        for (int i = 0; i < count; i++) {
            degrees[i] -= lowest;
        }
    }

    // Store into current edit step
    int editStep = clamp(v[kParamCurrentStep] - 1, 0, NUM_STEPS - 1);
    int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);

    StepState* ss = &alg->stepStates[editStep];
    ss->baseChord.count = (uint8_t)count;
    for (int i = 0; i < count; i++) {
        ss->baseChord.degrees[i] = (int8_t)clamp(degrees[i], -127, 127);
    }

    // Auto-advance edit step (wrap at stepCount)
    int nextStep = (editStep + 1) % stepCount;
    NT_setParameterFromAudio(NT_algorithmIndex(alg), kParamCurrentStep + NT_parameterOffset(), (int16_t)(nextStep + 1));

    // Reset snapshot for next capture
    dtc->snapshotCount = 0;
    memset(alg->snapshotNotes, 0, sizeof(alg->snapshotNotes));
}
