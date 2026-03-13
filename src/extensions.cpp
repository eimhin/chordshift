/*
 * Chordshift - Chord Extensions
 *
 * Adds diatonic extensions (7th, 9th, 11th, 13th) as scored candidates.
 * Each candidate gets a base priority plus clash penalties based on
 * semitone proximity to existing chord tones. Extensions are added
 * when colorAmount >= score.
 *
 * Fully deterministic — no randomness needed.
 */

#include "extensions.h"
#include "math.h"
#include "scales.h"

// Base priorities (lower = more likely to be added)
static constexpr int BASE_7TH  = 10;
static constexpr int BASE_9TH  = 20;
static constexpr int BASE_11TH = 40;  // Most clash-prone, harder to add than 13th
static constexpr int BASE_13TH = 30;

// Clash penalties
static constexpr int CLASH_1_SEMI = 50;
static constexpr int CLASH_2_SEMI = 15;

// Register penalty for low-voiced extensions
static constexpr int LOW_REGISTER_PENALTY = 20;

// Extension candidate degree offsets from root (in 7-note scale terms)
static constexpr int EXT_7TH  = 6;
static constexpr int EXT_9TH  = 8;
static constexpr int EXT_11TH = 10;
static constexpr int EXT_13TH = 12;

struct ExtCandidate {
    int16_t degree;
    int score;
};

void applyExtensions(DegreeBuffer* buf, const int16_t* v) {
    applyExtensions(buf, v, -1, -1);
}

void applyExtensions(DegreeBuffer* buf, const int16_t* v, int depthOverride, int colorOverride) {
    int depth = depthOverride >= 0 ? depthOverride : v[kParamExtDepth];
    int colorAmount = colorOverride >= 0 ? colorOverride : v[kParamExtColor];
    if (colorAmount < 0) colorAmount = 0;
    if (colorAmount > 100) colorAmount = 100;

    if (depth <= 0 || buf->count == 0) return;

    int scaleType = v[kParamScaleType];
    ScaleData sd = getScaleData(scaleType);
    int scaleLen = sd.length;
    int root = v[kParamScaleRoot];
    int octaveBase = v[kParamOctaveBase];

    // Find chord root (lowest degree)
    int16_t chordRoot = buf->degrees[0];
    for (int i = 1; i < buf->count; i++) {
        if (buf->degrees[i] < chordRoot) chordRoot = buf->degrees[i];
    }

    // Scale extension offsets for non-7-note scales
    // E.g. pentatonic (5 notes): 7th offset = 6*5/7 = 4
    static const int extOffsets7[] = {EXT_7TH, EXT_9TH, EXT_11TH, EXT_13TH};
    static const int basePriorities[] = {BASE_7TH, BASE_9TH, BASE_11TH, BASE_13TH};
    static constexpr int NUM_EXT = 4;

    // Build MIDI values for existing chord tones (for clash detection)
    int chordMidi[MAX_RENDER_NOTES];
    for (int i = 0; i < buf->count; i++) {
        chordMidi[i] = degreeToMidiSD(buf->degrees[i], root, octaveBase, sd);
    }

    // Build and score candidates
    ExtCandidate candidates[NUM_EXT];
    int numCandidates = 0;

    for (int e = 0; e < NUM_EXT && e < depth; e++) {
        // Scale offset to current scale length
        int offset = (extOffsets7[e] * scaleLen + 3) / 7;  // rounded
        int16_t candDeg = chordRoot + (int16_t)offset;

        // Check if this pitch class already exists in chord (mod scaleLen)
        // Normalize to positive mod
        int candPC = ((candDeg % scaleLen) + scaleLen) % scaleLen;
        bool duplicate = false;
        for (int i = 0; i < buf->count; i++) {
            int pc = ((buf->degrees[i] % scaleLen) + scaleLen) % scaleLen;
            if (pc == candPC) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        // Score: base priority + clash penalties
        int score = basePriorities[e];

        // Check semitone proximity to each chord tone
        int candMidi = degreeToMidiSD(candDeg, root, octaveBase, sd);
        for (int i = 0; i < buf->count; i++) {
            int diff = candMidi - chordMidi[i];
            // Reduce to pitch class distance (0-6 semitones)
            int absDiff = diff < 0 ? -diff : diff;
            int pcDist = absDiff % 12;
            if (pcDist > 6) pcDist = 12 - pcDist;

            if (pcDist == 1) score += CLASH_1_SEMI;
            else if (pcDist == 2) score += CLASH_2_SEMI;
        }

        // Register penalty: if candidate is in same octave as root
        if (candDeg < chordRoot + scaleLen) {
            score += LOW_REGISTER_PENALTY;
        }

        candidates[numCandidates].degree = candDeg;
        candidates[numCandidates].score = score;
        numCandidates++;
    }

    // Sort by score ascending (insertion sort, small array)
    for (int i = 1; i < numCandidates; i++) {
        ExtCandidate key = candidates[i];
        int j = i - 1;
        while (j >= 0 && candidates[j].score > key.score) {
            candidates[j + 1] = candidates[j];
            j--;
        }
        candidates[j + 1] = key;
    }

    // Add extensions where colorAmount >= score
    for (int i = 0; i < numCandidates && buf->count < MAX_RENDER_NOTES; i++) {
        if (colorAmount >= candidates[i].score) {
            buf->degrees[buf->count++] = candidates[i].degree;
        }
    }
}
