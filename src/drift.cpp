/*
 * Chordshift - Harmonic Drift
 *
 * Gradual, probabilistic chord substitution at runtime. Chords "drift" to
 * related diatonic degrees while preserving shape. The original progression
 * is never modified; drift is a runtime-only degree-space transpose per step.
 *
 * Styles:
 * - Neighbor: offset ±1
 * - Functional: members of same harmonic family (7-note scales only)
 * - Orbit: accumulates in one direction, ±1 per evaluation
 */

#include "drift.h"
#include "math.h"
#include "random.h"
#include "scales.h"

// ============================================================================
// INTERVAL CONVERSION
// ============================================================================

// Convert drift interval enum (0=Off, 1=4, 2=8, 3=16, 4=32, 5=64, 6=128)
// to step count. Returns 0 for Off.
static int getDriftIntervalSteps(int enumVal) {
    static const int table[] = {0, 4, 8, 16, 32, 64, 128};
    if (enumVal < 0 || enumVal > 6) return 0;
    return table[enumVal];
}

// ============================================================================
// HARMONIC FAMILY TABLES (for Functional style)
// ============================================================================

// Tonic family: I(0), iii(2), vi(5)
// Predominant family: ii(1), IV(3)
// Dominant family: V(4), vii(6)
static int getHarmonicFamily(int degreeMod7) {
    static const int family[] = {0, 1, 0, 1, 2, 0, 2};
    if (degreeMod7 < 0 || degreeMod7 > 6) return 0;
    return family[degreeMod7];
}

static const int TONIC_MEMBERS[] = {0, 2, 5};
static const int PREDOMINANT_MEMBERS[] = {1, 3};
static const int DOMINANT_MEMBERS[] = {4, 6};

struct FamilyMembers {
    const int* members;
    int count;
};

static FamilyMembers getFamilyMembers(int family) {
    FamilyMembers fm;
    switch (family) {
        case 0: fm.members = TONIC_MEMBERS; fm.count = 3; break;
        case 1: fm.members = PREDOMINANT_MEMBERS; fm.count = 2; break;
        default: fm.members = DOMINANT_MEMBERS; fm.count = 2; break;
    }
    return fm;
}

// ============================================================================
// CANDIDATE GENERATION
// ============================================================================

struct DriftCandidate {
    int8_t newOffset;
    int weight;  // higher = more likely
};

// Generate neighbor candidates: current ±1, weighted toward zero
static int generateNeighborCandidates(DriftCandidate* cands, int8_t current) {
    int n = 0;
    int8_t lo = current - 1;
    int8_t hi = current + 1;

    // Weight: closer to zero gets higher weight
    int loAbs = lo < 0 ? -lo : lo;
    int hiAbs = hi < 0 ? -hi : hi;

    cands[n].newOffset = lo;
    cands[n].weight = 10 - loAbs;
    if (cands[n].weight < 1) cands[n].weight = 1;
    n++;

    cands[n].newOffset = hi;
    cands[n].weight = 10 - hiAbs;
    if (cands[n].weight < 1) cands[n].weight = 1;
    n++;

    return n;
}

// Generate functional candidates: other members of same harmonic family
// Falls back to neighbor for non-7-note scales
static int generateFunctionalCandidates(DriftCandidate* cands, int8_t current,
                                         int scaleLen) {
    if (scaleLen != 7) {
        return generateNeighborCandidates(cands, current);
    }

    // Current offset mod 7 gives us the degree class shift
    int currentMod = ((current % 7) + 7) % 7;
    int family = getHarmonicFamily(currentMod);
    FamilyMembers fm = getFamilyMembers(family);

    int n = 0;
    for (int i = 0; i < fm.count && n < 6; i++) {
        // Compute offset that would shift to this family member
        int target = fm.members[i];
        int delta = target - currentMod;
        int8_t newOff = current + (int8_t)delta;

        if (newOff == current) continue;

        int absOff = newOff < 0 ? -newOff : newOff;
        cands[n].newOffset = newOff;
        cands[n].weight = 10 - absOff;
        if (cands[n].weight < 1) cands[n].weight = 1;
        n++;
    }

    // If no candidates (shouldn't happen), fall back to neighbor
    if (n == 0) return generateNeighborCandidates(cands, current);
    return n;
}

// Generate orbit candidates: always ±1 in same direction
static int generateOrbitCandidates(DriftCandidate* cands, int8_t current,
                                    uint32_t& randState) {
    // Determine direction: if at zero, pick random; otherwise continue in same direction
    int dir;
    if (current == 0) {
        dir = (nextRand(randState) & 1) ? 1 : -1;
    } else {
        dir = current > 0 ? 1 : -1;
    }

    int8_t newOff = current + (int8_t)dir;
    cands[0].newOffset = newOff;
    cands[0].weight = 1;
    return 1;
}

// ============================================================================
// WEIGHTED SELECTION
// ============================================================================

static int8_t weightedSelect(DriftCandidate* cands, int count, uint32_t& randState) {
    if (count <= 0) return 0;
    if (count == 1) return cands[0].newOffset;

    int totalWeight = 0;
    for (int i = 0; i < count; i++) totalWeight += cands[i].weight;
    if (totalWeight <= 0) return cands[0].newOffset;

    int roll = randRange(randState, 0, totalWeight - 1);
    int accum = 0;
    for (int i = 0; i < count; i++) {
        accum += cands[i].weight;
        if (roll < accum) return cands[i].newOffset;
    }
    return cands[count - 1].newOffset;
}

// ============================================================================
// PUBLIC API
// ============================================================================

void evaluateDrift(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    int intervalEnum = v[kParamDriftInterval];
    int intervalSteps = getDriftIntervalSteps(intervalEnum);
    if (intervalSteps == 0) return;  // Drift off

    int driftAmount = v[kParamDriftAmount];
    if (driftAmount <= 0) return;

    dtc->driftStepCounter++;
    if (dtc->driftStepCounter < (uint16_t)intervalSteps) return;
    dtc->driftStepCounter = 0;

    // Choose target step
    int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);
    int targetStep;
    int scope = v[kParamDriftScope];

    if (scope == 0) {
        // Focused: rotate through steps sequentially
        dtc->focusedDriftStep++;
        if (dtc->focusedDriftStep >= (int8_t)stepCount) {
            dtc->focusedDriftStep = 0;
        }
        targetStep = dtc->focusedDriftStep;
    } else {
        // Distributed: pick randomly
        targetStep = randRange(alg->randState, 0, stepCount - 1);
    }

    int8_t currentOffset = dtc->driftOffset[targetStep];

    // Return-or-drift decision: if already drifted, chance to move back toward 0
    if (currentOffset != 0) {
        int returnProb = 100 - driftAmount;
        if ((int)(randFloat(alg->randState) * 100.0f) < returnProb) {
            // Move one step toward zero
            if (currentOffset > 0) currentOffset--;
            else currentOffset++;
            dtc->driftOffset[targetStep] = currentOffset;
            return;
        }
    }

    // Generate candidates by style
    int style = v[kParamDriftStyle];
    int scaleType = v[kParamScaleType];
    ScaleData sd = getScaleData(scaleType);

    DriftCandidate cands[6];
    int numCands = 0;

    switch (style) {
        case 0:  // Neighbor
            numCands = generateNeighborCandidates(cands, currentOffset);
            break;
        case 1:  // Functional
            numCands = generateFunctionalCandidates(cands, currentOffset, sd.length);
            break;
        case 2:  // Orbit
            numCands = generateOrbitCandidates(cands, currentOffset, alg->randState);
            break;
        case 3:  // Color (same movement as Neighbor, applied differently in playback)
            numCands = generateNeighborCandidates(cands, currentOffset);
            break;
        default:
            numCands = generateNeighborCandidates(cands, currentOffset);
            break;
    }

    int8_t newOffset = weightedSelect(cands, numCands, alg->randState);

    // Bound to ±scaleLen: Orbit resets to 0, Color clamps, others wrap
    int len = sd.length;
    if (newOffset > len || newOffset < -len) {
        if (style == 2) {
            newOffset = 0;
        } else if (style == 3) {
            newOffset = newOffset > 0 ? len : -len;
        } else {
            newOffset = newOffset + (newOffset > 0 ? -2 * len : 2 * len);
        }
    }

    dtc->driftOffset[targetStep] = newOffset;
}

void applyDrift(DegreeBuffer* buf, int8_t offset) {
    if (offset == 0 || buf->count == 0) return;
    for (int i = 0; i < buf->count; i++) {
        buf->degrees[i] += offset;
    }
}

void resetDrift(Chordshift_DTC* dtc) {
    for (int i = 0; i < NUM_STEPS; i++) {
        dtc->driftOffset[i] = 0;
    }
    dtc->driftStepCounter = 0;
    dtc->focusedDriftStep = -1;
}
