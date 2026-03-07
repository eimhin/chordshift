#include "randomize.h"
#include "config.h"
#include "math.h"
#include "random.h"
#include <distingnt/api.h>

static const int8_t contourArc[NUM_STEPS]  = {-4, -1,  2,  4,  5,  3,  0, -3};
static const int8_t contourRise[NUM_STEPS] = {-5, -3, -2,  0,  1,  2,  4,  5};
static const int8_t contourFall[NUM_STEPS] = { 5,  4,  2,  1,  0, -2, -3, -5};

static const int seqLenValues[] = {0, 8, 16, 32, 64, 128, 256};
static const int seqDivValues[] = {1, 1, 2, 4, 8, 16, 32};

static void randomizeSequenceLength(uint32_t& randState, const int16_t* v,
                                    uint32_t idx, uint32_t off) {
    int seqLenEnum = clamp(v[kParamRandSeqLen], 0, 6);
    if (seqLenEnum == 0) return;

    int seqLen = seqLenValues[seqLenEnum];
    int seqDiv = seqDivValues[clamp(v[kParamRandSeqDiv], 0, 6)];
    bool uniform = v[kParamRandSeqHold] == 1;

    // Build list of valid clockDiv indices (where holdUnit <= 8)
    int validDivs[NUM_CLOCK_DIV_VALUES];
    int numValidDivs = 0;
    for (int i = 0; i < NUM_CLOCK_DIV_VALUES; i++) {
        int cd = CLOCK_DIV_VALUES[i];
        int hu = seqDiv / gcd(cd, seqDiv);
        if (hu <= 8)
            validDivs[numValidDivs++] = i;
    }
    if (numValidDivs == 0) return;

    // Retry loop
    for (int attempt = 0; attempt < 32; attempt++) {
        int divIdx = validDivs[randRange(randState, 0, numValidDivs - 1)];
        int cd = CLOCK_DIV_VALUES[divIdx];
        int holdUnit = seqDiv / gcd(cd, seqDiv);
        int maxHoldPerStep = (8 / holdUnit) * holdUnit;
        int steps = randRange(randState, 1, NUM_STEPS);

        int minSum = steps * holdUnit;
        int maxSum = steps * maxHoldPerStep;

        // Enumerate valid holdSums
        int validSums[8];
        int numValidSums = 0;
        for (int reps = 1; numValidSums < 8; reps++) {
            int total = cd * reps;
            if (total > seqLen) break;
            if (seqLen % total != 0) continue;
            int holdSum = seqLen / total;
            if (holdSum < minSum || holdSum > maxSum) continue;
            if (holdSum % holdUnit != 0) continue;
            if (uniform) {
                // holdSum must be divisible by steps, and per-step hold must be valid
                if (holdSum % steps != 0) continue;
                int perStep = holdSum / steps;
                if (perStep % holdUnit != 0) continue;
                if (perStep < 1 || perStep > 8) continue;
            }
            validSums[numValidSums++] = holdSum;
        }
        if (numValidSums == 0) continue;

        int holdSum = validSums[randRange(randState, 0, numValidSums - 1)];

        // Apply steps and clockDiv
        NT_setParameterFromAudio(idx, kParamStepCount + off, steps);
        NT_setParameterFromAudio(idx, kParamClockDiv + off, divIdx);

        if (uniform) {
            int perStep = holdSum / steps;
            for (int s = 0; s < NUM_STEPS; s++)
                NT_setParameterFromAudio(idx, stepParam(s, kStepHold) + off,
                                         s < steps ? perStep : 1);
        } else {
            // Varied: start each step at holdUnit, distribute bonus units
            int holds[NUM_STEPS];
            for (int s = 0; s < steps; s++)
                holds[s] = holdUnit;
            int bonusUnits = (holdSum / holdUnit) - steps;
            for (int b = 0, retries = 0; b < bonusUnits && retries < 256; retries++) {
                int s = randRange(randState, 0, steps - 1);
                if (holds[s] + holdUnit <= 8) {
                    holds[s] += holdUnit;
                    b++;
                }
            }
            for (int s = 0; s < NUM_STEPS; s++)
                NT_setParameterFromAudio(idx, stepParam(s, kStepHold) + off,
                                         s < steps ? holds[s] : 1);
        }
        return;
    }
}

void randomizeSequence(uint32_t& randState, const int16_t* v,
                       uint32_t idx, uint32_t off) {
    int contour = v[kParamRandomContour];

    // Sequence length randomization (Steps, ClockDiv, Hold)
    randomizeSequenceLength(randState, v, idx, off);

    int dTemplate  = clamp(v[kParamRandTemplate],  0, 100);
    int dTranspose = clamp(v[kParamRandTranspose], 0, 100);
    int dInversion = clamp(v[kParamRandInversion], 0, 100);
    int dRotation  = clamp(v[kParamRandRotation],  0, 100);
    int dSpread    = clamp(v[kParamRandSpread],    0, 100);
    int dReverse   = clamp(v[kParamRandReverse],   0, 100);
    int dGate      = clamp(v[kParamRandGate],      0, 100);
    int dRepeat    = clamp(v[kParamRandRepeat],    0, 100);

    for (int s = 0; s < NUM_STEPS; s++) {
        // Template
        if (dTemplate > 0) {
            int tmpl = randRange(randState, 3, 4);
            NT_setParameterFromAudio(idx, stepParam(s, kStepTemplate) + off, tmpl);
        }

        // Transpose: contour-controlled, range scaled by depth
        if (dTranspose > 0) {
            int transpose;
            int range = (5 * dTranspose + 50) / 100;
            switch (contour) {
            case 1: // Arc
                transpose = clamp(contourArc[s] + randRange(randState, -1, 1), -range, range);
                break;
            case 2: // Rise
                transpose = clamp(contourRise[s] + randRange(randState, -1, 1), -range, range);
                break;
            case 3: // Fall
                transpose = clamp(contourFall[s] + randRange(randState, -1, 1), -range, range);
                break;
            default: // Random
                transpose = randRange(randState, -range, range);
                break;
            }
            NT_setParameterFromAudio(idx, stepParam(s, kStepTranspose) + off, transpose);
        }

        // Inversion
        if (dInversion > 0) {
            int invRange = (4 * dInversion + 50) / 100;
            NT_setParameterFromAudio(idx, stepParam(s, kStepInversion) + off,
                                     randRange(randState, -invRange, invRange));
        }

        // Rotation
        if (dRotation > 0) {
            int rotRange = (7 * dRotation + 50) / 100;
            NT_setParameterFromAudio(idx, stepParam(s, kStepRotation) + off,
                                     randRange(randState, -rotRange, rotRange));
        }

        // Spread
        if (dSpread > 0) {
            int sprRange = (7 * dSpread + 50) / 100;
            NT_setParameterFromAudio(idx, stepParam(s, kStepSpread) + off,
                                     randRange(randState, -sprRange, sprRange));
        }

        // Reverse: depth as probability
        if (dReverse > 0) {
            int rev = (randRange(randState, 0, 99) < dReverse) ? 1 : 0;
            NT_setParameterFromAudio(idx, stepParam(s, kStepReverse) + off, rev);
        }

        // Gate: 1..round(1 + 199 * depth/100)
        if (dGate > 0) {
            int gateMax = 1 + (199 * dGate + 50) / 100;
            NT_setParameterFromAudio(idx, stepParam(s, kStepGateLength) + off,
                                     randRange(randState, 1, gateMax));
        }

        // Repeat: 1..round(1 + 3 * depth/100)
        if (dRepeat > 0) {
            int repMax = 1 + (3 * dRepeat + 50) / 100;
            NT_setParameterFromAudio(idx, stepParam(s, kStepRepeat) + off,
                                     randRange(randState, 1, repMax));
        }
    }
}
