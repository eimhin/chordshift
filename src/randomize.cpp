#include "randomize.h"
#include "config.h"
#include "math.h"
#include "random.h"
#include <distingnt/api.h>

static const int8_t contourArc[NUM_STEPS]  = {-4, -1,  2,  4,  5,  3,  0, -3};
static const int8_t contourRise[NUM_STEPS] = {-5, -3, -2,  0,  1,  2,  4,  5};
static const int8_t contourFall[NUM_STEPS] = { 5,  4,  2,  1,  0, -2, -3, -5};

void randomizeSequence(uint32_t& randState, const int16_t* v,
                       uint32_t idx, uint32_t off) {
    int contour = v[kParamRandomContour];
    int depth = clamp(v[kParamRandomDepth], 0, 100);

    // Randomize step count 1-8
    int stepCount = randRange(randState, 1, NUM_STEPS);
    NT_setParameterFromAudio(idx, kParamStepCount + off, stepCount);

    for (int s = 0; s < NUM_STEPS; s++) {
        // Template: always Triad(3) or 7th(4)
        int tmpl = randRange(randState, 3, 4);
        NT_setParameterFromAudio(idx, stepParam(s, kStepTemplate) + off, tmpl);

        // Transpose: contour-controlled
        int transpose;
        switch (contour) {
        case 1: // Arc
            transpose = clamp(contourArc[s] + randRange(randState, -1, 1), -5, 5);
            break;
        case 2: // Rise
            transpose = clamp(contourRise[s] + randRange(randState, -1, 1), -5, 5);
            break;
        case 3: // Fall
            transpose = clamp(contourFall[s] + randRange(randState, -1, 1), -5, 5);
            break;
        default: // Random
            transpose = randRange(randState, -5, 5);
            break;
        }
        NT_setParameterFromAudio(idx, stepParam(s, kStepTranspose) + off, transpose);

        // Depth-scaled parameters
        int invRange = (4 * depth + 50) / 100;
        NT_setParameterFromAudio(idx, stepParam(s, kStepInversion) + off,
                                 randRange(randState, -invRange, invRange));

        int rotRange = (7 * depth + 50) / 100;
        NT_setParameterFromAudio(idx, stepParam(s, kStepRotation) + off,
                                 randRange(randState, -rotRange, rotRange));

        int sprRange = (7 * depth + 50) / 100;
        NT_setParameterFromAudio(idx, stepParam(s, kStepSpread) + off,
                                 randRange(randState, -sprRange, sprRange));

        // Reverse: depth as probability
        int rev = (randRange(randState, 0, 99) < depth) ? 1 : 0;
        NT_setParameterFromAudio(idx, stepParam(s, kStepReverse) + off, rev);

        // Gate: 1..round(1 + 199 * depth/100)
        int gateMax = 1 + (199 * depth + 50) / 100;
        NT_setParameterFromAudio(idx, stepParam(s, kStepGateLength) + off,
                                 randRange(randState, 1, gateMax));

        // Repeat: 1..round(1 + 3 * depth/100)
        int repMax = 1 + (3 * depth + 50) / 100;
        NT_setParameterFromAudio(idx, stepParam(s, kStepRepeat) + off,
                                 randRange(randState, 1, repMax));

        // Hold: 1..round(1 + 7 * depth/100)
        int holdMax = 1 + (7 * depth + 50) / 100;
        NT_setParameterFromAudio(idx, stepParam(s, kStepHold) + off,
                                 randRange(randState, 1, holdMax));
    }
}
