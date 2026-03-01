/*
 * MIDI Chords - Render Pipeline
 *
 * Converts transformed DegreeBuffer to RenderedChord:
 * 1. degreeToMidi() for each degree
 * 2. Apply velocity curve scaled by depth
 * 3. Apply strum delay (index * strumTime)
 * 4. Apply ratchet (repeat N times with time offset)
 * 5. Calculate gate length (stepDuration * gateLength%)
 * 6. Clamp all values to valid MIDI ranges
 */

#include "render.h"
#include "math.h"
#include "random.h"
#include "scales.h"

// ============================================================================
// VELOCITY CURVE
// ============================================================================

// Apply velocity curve to a note based on its position in the chord.
// Returns velocity offset (can be positive or negative).
static int velocityCurveOffset(int index, int count, int curve, int depth, uint32_t& randState) {
    if (depth == 0 || count <= 1) return 0;

    float t = (float)index / (float)(count - 1);  // 0.0 to 1.0
    float curveVal = 0.0f;

    switch (curve) {
        case VCURVE_LINEAR:
            curveVal = t;
            break;
        case VCURVE_EXP:
            curveVal = t * t;
            break;
        case VCURVE_TRIANGLE:
            curveVal = (t < 0.5f) ? (2.0f * t) : (2.0f * (1.0f - t));
            break;
        case VCURVE_SQUARE:
            curveVal = (t < 0.5f) ? 1.0f : 0.0f;
            break;
        case VCURVE_RANDOM:
            curveVal = randFloat(randState);
            break;
    }

    // Scale by depth percentage, center around 0
    // depth=100 means full range (-64 to +64 effectively)
    float scaled = (curveVal - 0.5f) * 2.0f * ((float)depth / 100.0f) * 64.0f;
    return (int)scaled;
}

// ============================================================================
// RENDER
// ============================================================================

void renderChord(RenderedChord* out, const DegreeBuffer* buf, const int16_t* v,
                 int stepIdx, float stepDuration, uint32_t& randState) {
    out->count = 0;
    if (buf->count == 0) return;

    StepParams sp = StepParams::fromAlgorithm(v, stepIdx);

    int root = v[kParamScaleRoot];
    int octaveBase = v[kParamOctaveBase];
    ScaleData sd = getScaleData(v[kParamScaleType]);
    int baseVelocity = v[kParamBaseVelocity];
    int velCurve = v[kParamVelCurve];
    int velDepth = v[kParamVelDepth];
    int strumTime = v[kParamStrumTime] + sp.strumTime();  // additive
    int velOffset = sp.velocity();  // per-step velocity offset
    int gatePercent = sp.gateLength();
    int ratchetCount = sp.repeat();

    // Calculate gate time in ms
    uint16_t gateMs = (uint16_t)clamp((int)(stepDuration * 1000.0f * gatePercent / 100), 1, 60000);

    // For ratchet, divide gate time among repeats
    uint16_t ratchetGateMs = gateMs;
    uint16_t ratchetInterval = 0;
    if (ratchetCount > 1) {
        ratchetInterval = gateMs / (uint16_t)ratchetCount;
        ratchetGateMs = ratchetInterval > 1 ? ratchetInterval - 1 : 1;
    }

    for (int r = 0; r < ratchetCount; r++) {
        uint16_t ratchetOffset = (uint16_t)(r * ratchetInterval);

        for (int i = 0; i < buf->count && out->count < MAX_RENDER_NOTES; i++) {
            // Convert degree to MIDI
            int midi = degreeToMidiSD(buf->degrees[i], root, octaveBase, sd);
            midi = clamp(midi, 0, 127);

            // Calculate velocity
            int vel = baseVelocity + velOffset;
            vel += velocityCurveOffset(i, buf->count, velCurve, velDepth, randState);
            vel = clamp(vel, 1, 127);

            // Calculate strum delay
            uint16_t delay = (uint16_t)clamp(i * strumTime, 0, 10000);
            delay += ratchetOffset;

            RenderedNote* rn = &out->notes[out->count];
            rn->midiNote = (uint8_t)midi;
            rn->velocity = (uint8_t)vel;
            rn->delayMs = delay;
            rn->gateMs = ratchetGateMs;
            out->count++;
        }
    }
}
