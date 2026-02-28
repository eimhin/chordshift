/*
 * MIDI Chords - Capture Pipeline
 *
 * When Record=On, incoming MIDI notes are accumulated. On each note-on,
 * snapshot the held notes (capturing largest chord). When last note released,
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

// ============================================================================
// CAPTURE NOTE ON
// ============================================================================

void captureNoteOn(MidiChordsAlgorithm* alg, uint8_t note, uint8_t velocity) {
    MidiChords_DTC* dtc = alg->dtc;

    // Track held notes
    dtc->captureNotes[note] = 1;
    dtc->captureCount++;
    dtc->inputVel = velocity;

    // Snapshot current held notes if this is a new peak
    if (dtc->captureCount >= dtc->snapshotCount) {
        for (int i = 0; i < 128; i++) {
            dtc->snapshotNotes[i] = dtc->captureNotes[i];
        }
        dtc->snapshotCount = dtc->captureCount;
    }
}

// ============================================================================
// CAPTURE NOTE OFF — COMMIT CHORD WHEN LAST NOTE RELEASED
// ============================================================================

void captureNoteOff(MidiChordsAlgorithm* alg, uint8_t note) {
    MidiChords_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    if (dtc->captureNotes[note] == 0) return;
    dtc->captureNotes[note] = 0;
    if (dtc->captureCount > 0) dtc->captureCount--;

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
        if (dtc->snapshotNotes[n]) {
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
    int nextStep = editStep + 1;
    if (nextStep >= stepCount) nextStep = 0;
    // We can't directly set parameter values from code on disting NT,
    // so we just reset the snapshot for the next capture
    // The user will manually advance or we track internally

    // Reset snapshot for next capture
    dtc->snapshotCount = 0;
    for (int i = 0; i < 128; i++) {
        dtc->snapshotNotes[i] = 0;
    }
}
