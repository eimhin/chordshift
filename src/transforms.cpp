/*
 * MIDI Chords - Transform Pipeline
 *
 * Operates on DegreeBuffer in-place. Combines global + per-step parameters.
 *
 * Stage 1 — Pitch: Transpose, Reflect, Spread
 * Stage 2 — Voicing: Inversion, Rotation
 * Stage 3 — Normalize: None / LowestTo0 / FirstTo0
 * Stage 4 — Order: Reverse, Direction
 *
 * Combination rules:
 * - Transpose, Inversion, Rotation, Spread, Strum: additive (global + step)
 * - Reverse: XOR (global ^ step)
 * - Reflect: override (step replaces global if nonzero)
 */

#include "transforms.h"
#include "math.h"
#include "random.h"
#include "scales.h"

// ============================================================================
// STAGE 1: PITCH TRANSFORMS
// ============================================================================

static void applyTranspose(DegreeBuffer* buf, int amount) {
    if (amount == 0) return;
    for (int i = 0; i < buf->count; i++) {
        buf->degrees[i] += (int16_t)amount;
    }
}

static void applyReflect(DegreeBuffer* buf, int mode) {
    if (mode == REFLECT_OFF || buf->count == 0) return;

    int16_t axis = 0;
    switch (mode) {
        case REFLECT_ROOT:
            axis = 0;
            break;
        case REFLECT_LOWEST:
            axis = buf->degrees[0];
            for (int i = 1; i < buf->count; i++) {
                if (buf->degrees[i] < axis) axis = buf->degrees[i];
            }
            break;
        case REFLECT_HIGHEST:
            axis = buf->degrees[0];
            for (int i = 1; i < buf->count; i++) {
                if (buf->degrees[i] > axis) axis = buf->degrees[i];
            }
            break;
    }

    for (int i = 0; i < buf->count; i++) {
        buf->degrees[i] = 2 * axis - buf->degrees[i];
    }
}

static void applySpread(DegreeBuffer* buf, int amount, int anchor) {
    if (amount == 0 || buf->count <= 1) return;

    if (anchor == ANCHOR_LOWEST) {
        // Anchor at index 0: d' = d + spread * index
        for (int i = 0; i < buf->count; i++) {
            buf->degrees[i] += (int16_t)(amount * i);
        }
    } else {
        // Anchor at center: d' = d + spread * (index - center)
        // Use half-count as center point (integer division)
        int center = (buf->count - 1) / 2;
        for (int i = 0; i < buf->count; i++) {
            buf->degrees[i] += (int16_t)(amount * (i - center));
        }
    }
}

// ============================================================================
// STAGE 2: VOICING TRANSFORMS
// ============================================================================

static void applyInversion(DegreeBuffer* buf, int inv, int scaleType) {
    if (inv == 0 || buf->count <= 1) return;

    ScaleData sd = getScaleData(scaleType);
    int scaleLen = sd.length;

    if (inv > 0) {
        // Positive inversion: move lowest note up by scaleLen
        for (int i = 0; i < inv && buf->count > 1; i++) {
            // Find lowest
            int lowestIdx = 0;
            for (int j = 1; j < buf->count; j++) {
                if (buf->degrees[j] < buf->degrees[lowestIdx]) lowestIdx = j;
            }
            buf->degrees[lowestIdx] += (int16_t)scaleLen;
        }
    } else {
        // Negative inversion: move highest note down by scaleLen
        for (int i = 0; i < -inv && buf->count > 1; i++) {
            int highestIdx = 0;
            for (int j = 1; j < buf->count; j++) {
                if (buf->degrees[j] > buf->degrees[highestIdx]) highestIdx = j;
            }
            buf->degrees[highestIdx] -= (int16_t)scaleLen;
        }
    }
}

static void applyRotation(DegreeBuffer* buf, int rot) {
    if (rot == 0 || buf->count <= 1) return;

    int n = buf->count;
    // Normalize rotation to [0, n)
    int r = ((rot % n) + n) % n;
    if (r == 0) return;

    // Simple rotation using temp buffer
    int16_t temp[MAX_RENDER_NOTES];
    for (int i = 0; i < n; i++) {
        temp[i] = buf->degrees[(i + r) % n];
    }
    for (int i = 0; i < n; i++) {
        buf->degrees[i] = temp[i];
    }
}

// ============================================================================
// STAGE 3: NORMALIZE
// ============================================================================

static void applyNormalize(DegreeBuffer* buf, int mode) {
    if (mode == NORM_NONE || buf->count == 0) return;

    int16_t offset = 0;
    if (mode == NORM_LOWEST_TO_0) {
        offset = buf->degrees[0];
        for (int i = 1; i < buf->count; i++) {
            if (buf->degrees[i] < offset) offset = buf->degrees[i];
        }
    } else if (mode == NORM_FIRST_TO_0) {
        offset = buf->degrees[0];
    }

    if (offset != 0) {
        for (int i = 0; i < buf->count; i++) {
            buf->degrees[i] -= offset;
        }
    }
}

// ============================================================================
// STAGE 4: ORDER TRANSFORMS
// ============================================================================

static void applyReverse(DegreeBuffer* buf) {
    if (buf->count <= 1) return;
    for (int i = 0; i < buf->count / 2; i++) {
        int16_t tmp = buf->degrees[i];
        buf->degrees[i] = buf->degrees[buf->count - 1 - i];
        buf->degrees[buf->count - 1 - i] = tmp;
    }
}

static void applyDirection(DegreeBuffer* buf, int dir, uint32_t& randState) {
    if (buf->count <= 1) return;

    int16_t temp[MAX_RENDER_NOTES];
    int n = buf->count;

    switch (dir) {
        case DIR_FORWARD:
            // Already in order
            break;

        case DIR_BACKWARD:
            applyReverse(buf);
            break;

        case DIR_PINGPONG:
            // Forward then backward (without repeating endpoints)
            // [0,1,2,3] -> [0,1,2,3,2,1]
            {
                int newCount = n > 1 ? 2 * n - 2 : n;
                if (newCount > MAX_RENDER_NOTES) newCount = MAX_RENDER_NOTES;
                for (int i = 0; i < n && i < newCount; i++) {
                    temp[i] = buf->degrees[i];
                }
                for (int i = n; i < newCount; i++) {
                    temp[i] = buf->degrees[2 * (n - 1) - i];
                }
                for (int i = 0; i < newCount; i++) {
                    buf->degrees[i] = temp[i];
                }
                buf->count = (uint8_t)newCount;
            }
            break;

        case DIR_INSIDE_OUT:
            // From center outward: [0,1,2,3] -> [1,2,0,3] or [2,1,3,0]
            {
                int mid = n / 2;
                int idx = 0;
                if (n % 2 == 1) {
                    temp[idx++] = buf->degrees[mid];
                }
                for (int i = 1; i <= mid && idx < MAX_RENDER_NOTES; i++) {
                    if (mid - i >= 0 && idx < MAX_RENDER_NOTES) temp[idx++] = buf->degrees[mid - i];
                    if (mid + i < n && idx < MAX_RENDER_NOTES) temp[idx++] = buf->degrees[mid + i];
                }
                for (int i = 0; i < idx; i++) {
                    buf->degrees[i] = temp[i];
                }
                buf->count = (uint8_t)idx;
            }
            break;

        case DIR_OUTSIDE_IN:
            // From edges inward: [0,1,2,3] -> [0,3,1,2]
            {
                int idx = 0;
                int lo = 0, hi = n - 1;
                while (lo <= hi && idx < MAX_RENDER_NOTES) {
                    temp[idx++] = buf->degrees[lo++];
                    if (lo <= hi && idx < MAX_RENDER_NOTES) {
                        temp[idx++] = buf->degrees[hi--];
                    }
                }
                for (int i = 0; i < idx; i++) {
                    buf->degrees[i] = temp[i];
                }
                buf->count = (uint8_t)idx;
            }
            break;

        case DIR_RANDOM:
            // Fisher-Yates shuffle
            for (int i = n - 1; i > 0; i--) {
                int j = randRange(randState, 0, i);
                int16_t tmp = buf->degrees[i];
                buf->degrees[i] = buf->degrees[j];
                buf->degrees[j] = tmp;
            }
            break;
    }
}

// ============================================================================
// FULL PIPELINE
// ============================================================================

void applyTransforms(DegreeBuffer* buf, const int16_t* v, int stepIdx, uint32_t& randState) {
    if (buf->count == 0) return;

    StepParams sp = StepParams::fromAlgorithm(v, stepIdx);

    // Combine global + per-step parameters
    int transpose = v[kParamTranspose] + sp.transpose();       // additive
    int inversion = v[kParamInversion] + sp.inversion();       // additive
    int rotation = v[kParamRotation] + sp.rotation();          // additive
    int spread = v[kParamSpreadAmount] + sp.spread();          // additive
    int anchor = v[kParamSpreadAnchor];

    // Reflect: step overrides global if nonzero
    int reflect = v[kParamReflectMode];
    if (sp.reflect() != 0) reflect = sp.reflect();

    // Reverse: XOR
    bool reverse = (v[kParamReverse] == 1) ^ sp.reverse();

    int normalize = v[kParamNormalize];
    int direction = v[kParamDirection];
    int scaleType = v[kParamScaleType];

    // Stage 1: Pitch
    applyTranspose(buf, transpose);
    applyReflect(buf, reflect);
    applySpread(buf, spread, anchor);

    // Stage 2: Voicing
    applyInversion(buf, inversion, scaleType);
    applyRotation(buf, rotation);

    // Stage 3: Normalize
    applyNormalize(buf, normalize);

    // Stage 4: Order
    if (reverse) applyReverse(buf);
    applyDirection(buf, direction, randState);
}
