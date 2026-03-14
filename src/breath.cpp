/*
 * Chordshift - Voice Breathing
 *
 * Cyclically moves inner chord voices by ±1–2 scale degrees over time,
 * creating subtle voicing movement. Operates in degree space, applied after
 * the transform pipeline and before render.
 *
 * Built-in minimum voice gap of 2 degrees prevents diatonic seconds.
 */

#include "breath.h"
#include "math.h"
#include "random.h"

// ============================================================================
// CONSTANTS
// ============================================================================

static constexpr int BREATH_TABLE_LEN = 8;
static constexpr int BREATH_SHAPE_RANDOM = 3;
static constexpr int BREATH_SHAPE_WALK = 5;

// Scope modes
static constexpr int SCOPE_ALL_INNER = 0;
static constexpr int SCOPE_RAND_VOICE = 1;
static constexpr int SCOPE_TOP_ONLY = 2;
static constexpr int SCOPE_CONTRARY = 3;

// ============================================================================
// WAVE TABLES (16 table-driven shapes, values in [-2, +2])
// ============================================================================

static const int8_t breathTables[][BREATH_TABLE_LEN] = {
    { 0,  1,  2,  1,  0, -1, -2, -1},  //  0: Triangle
    { 2,  2,  2,  2, -2, -2, -2, -2},  //  1: Square
    {-2, -1, -1,  0,  0,  1,  1,  2},  //  2: Ramp
    { 0,  0,  0,  0,  0,  0,  0,  0},  //  3: Random (placeholder, runtime-generated)
    { 0,  1,  2,  2,  1,  0,  0,  0},  //  4: Pendulum
    { 0,  0,  0,  0,  0,  0,  0,  0},  //  5: Walk (placeholder, runtime-generated)
    { 0,  0,  2,  0,  0,  0, -2,  0},  //  6: Pulse
    { 2,  2,  1,  1,  0,  0,  0,  0},  //  7: Sigh
    { 0,  0,  0,  0,  1,  1,  2,  2},  //  8: Bloom
    { 2, -2,  2, -2,  2, -2,  2, -2},  //  9: Alternate
    { 2, -2,  2, -1,  1, -1,  0,  0},  // 10: Converge
    { 0,  2,  0,  2,  0,  2,  0, -1},  // 11: Return
    { 0,  0,  1,  1,  2,  1,  0, -1},  // 12: Sentence
    { 0,  1,  2, -1,  0,  1,  2, -1},  // 13: Period
    {-1,  0,  1,  2,  2,  1,  0, -1},  // 14: Arc
    { 0,  1,  2,  2,  2,  2,  1,  0},  // 15: Suspend
    { 0,  0,  1,  1,  0,  0, -1,  0},  // 16: Drift
    { 2,  2,  2,  1, -1, -2, -2, -2},  // 17: Tide
};

// ============================================================================
// INTERVAL CONVERSION
// ============================================================================


// ============================================================================
// OFFSET COMPUTATION
// ============================================================================

static int computeBreathOffset(int shape, int amount, int phase, int period,
                               Chordshift_DTC* dtc, uint32_t& randState) {
    if (shape == BREATH_SHAPE_RANDOM) {
        return randRange(randState, -amount, amount);
    }

    if (shape == BREATH_SHAPE_WALK) {
        int step = randRange(randState, -1, 1);
        int newOff = dtc->breathWalkOffset + step;
        newOff = clamp(newOff, -amount, amount);
        dtc->breathWalkOffset = (int8_t)newOff;
        return newOff;
    }

    // Table-driven shapes
    int idx = phase * BREATH_TABLE_LEN / period;
    if (idx >= BREATH_TABLE_LEN) idx = BREATH_TABLE_LEN - 1;
    int raw = breathTables[shape][idx];
    return clamp(raw, -amount, amount);
}

// ============================================================================
// GAP CHECK
// ============================================================================

// Returns true if applying offset to voice at voiceIdx would violate the
// minimum gap against any other voice in the buffer.
static bool wouldViolateGap(const int16_t* degrees, int count, int voiceIdx, int offset, int minGap) {
    if (minGap <= 0) return false;
    int newDeg = degrees[voiceIdx] + offset;
    for (int i = 0; i < count; i++) {
        if (i == voiceIdx) continue;
        int other = degrees[i];
        int diff = newDeg - other;
        if (diff < 0) diff = -diff;
        if (diff < minGap) return true;
    }
    return false;
}

// ============================================================================
// SORTING HELPERS
// ============================================================================

// Simple insertion sort of voice indices by pitch (ascending)
static void sortVoicesByPitch(const int16_t* degrees, int count, int* sorted) {
    for (int i = 0; i < count; i++) sorted[i] = i;
    for (int i = 1; i < count; i++) {
        int key = sorted[i];
        int keyVal = degrees[key];
        int j = i - 1;
        while (j >= 0 && degrees[sorted[j]] > keyVal) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

void applyBreathing(DegreeBuffer* buf, const int16_t* v, Chordshift_DTC* dtc, uint32_t& randState) {
    int amount = v[kParamBreathAmount];
    if (amount <= 0) return;

    int period = v[kParamBreathRate];
    if (period <= 0) return;

    if (buf->count <= 1) return;

    // Advance counter (wraps at period)
    int phase = dtc->breathStepCounter;
    dtc->breathStepCounter++;
    if (dtc->breathStepCounter >= (uint16_t)period) {
        dtc->breathStepCounter = 0;
    }

    int shape = clamp((int)v[kParamBreathShape], 0, 17);
    int scope = clamp((int)v[kParamBreathScope], 0, 3);
    int minGap = v[kParamBreathGap];

    int offset = computeBreathOffset(shape, amount, phase, period, dtc, randState);
    if (offset == 0) return;

    // Sort voices by pitch to identify lowest/highest/inner
    int sorted[MAX_RENDER_NOTES];
    sortVoicesByPitch(buf->degrees, buf->count, sorted);

    int lowestIdx = sorted[0];
    int highestIdx = sorted[buf->count - 1];

    switch (scope) {
        case SCOPE_ALL_INNER:
            // Apply offset to all voices except the lowest
            for (int i = 1; i < buf->count; i++) {
                int vi = sorted[i];
                if (!wouldViolateGap(buf->degrees, buf->count, vi, offset, minGap)) {
                    buf->degrees[vi] += (int16_t)offset;
                }
            }
            break;

        case SCOPE_RAND_VOICE: {
            // Pick one random non-lowest voice
            if (buf->count <= 1) break;
            int pick = randRange(randState, 1, buf->count - 1);
            int vi = sorted[pick];
            if (!wouldViolateGap(buf->degrees, buf->count, vi, offset, minGap)) {
                buf->degrees[vi] += (int16_t)offset;
            }
            break;
        }

        case SCOPE_TOP_ONLY:
            // Apply only to the highest voice
            if (!wouldViolateGap(buf->degrees, buf->count, highestIdx, offset, minGap)) {
                buf->degrees[highestIdx] += (int16_t)offset;
            }
            break;

        case SCOPE_CONTRARY:
            // Alternating inner voices get inverted offsets
            // 1st inner: +offset, 2nd inner: -offset, 3rd: +offset, etc.
            for (int i = 1; i < buf->count; i++) {
                int vi = sorted[i];
                int voiceOffset = ((i - 1) % 2 == 0) ? offset : -offset;
                if (!wouldViolateGap(buf->degrees, buf->count, vi, voiceOffset, minGap)) {
                    buf->degrees[vi] += (int16_t)voiceOffset;
                }
            }
            break;
    }

    (void)lowestIdx;  // Used implicitly via sorted[0] skip
}

void resetBreath(Chordshift_DTC* dtc) {
    dtc->breathStepCounter = 0;
    dtc->breathWalkOffset = 0;
}
