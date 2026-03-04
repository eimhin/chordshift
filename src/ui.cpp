/*
 * Chordshift - Custom UI
 *
 * 256x64 pixel display:
 * - Row 0-8: Standard param line (returned by draw() returning false)
 * - Row 10-28: 8 step boxes showing chord info
 * - Row 30-62: Current playing chord visualization
 */

#include "ui.h"
#include "math.h"
#include "scales.h"

// Note name table
static const char* const noteNames[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

// ============================================================================
// STEP GRID
// ============================================================================

static void drawDegreeGrid(int bx, int by, int boxWidth, const ChordDegrees* chord, int brightness) {
    static constexpr int SQ = 3;   // square size
    static constexpr int GAP = 1;  // gap between squares
    static constexpr int COLS = 6; // max cells per row

    // Find degree range
    int lo = chord->degrees[0], hi = chord->degrees[0];
    for (int i = 1; i < chord->count; i++) {
        if (chord->degrees[i] < lo) lo = chord->degrees[i];
        if (chord->degrees[i] > hi) hi = chord->degrees[i];
    }
    int span = hi - lo + 1;
    if (span > 18) span = 18;

    // Build presence mask
    bool present[18] = {};
    for (int i = 0; i < chord->count; i++) {
        int idx = chord->degrees[i] - lo;
        if (idx >= 0 && idx < 18)
            present[idx] = true;
    }

    // Calculate grid dimensions for centering
    int cols = (span < COLS) ? span : COLS;
    int rows = (span + COLS - 1) / COLS;
    int gridW = cols * SQ + (cols - 1) * GAP;
    int gridH = rows * SQ + (rows - 1) * GAP;
    int ox = bx + (boxWidth - gridW) / 2;
    int oy = by + (UI_STEP_HEIGHT - gridH) / 2;

    for (int i = 0; i < span; i++) {
        int col = i % COLS;
        int row = i / COLS;
        int x = ox + col * (SQ + GAP);
        int y = oy + row * (SQ + GAP);
        int b = present[i] ? brightness : 1;
        NT_drawShapeI(kNT_rectangle, x, y, x + SQ - 1, y + SQ - 1, b);
    }
}

static void drawStepGrid(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;
    const int16_t* v = alg->v;

    int stepCount = clamp(v[kParamStepCount], 1, NUM_STEPS);
    int editStep = clamp(v[kParamCurrentStep] - 1, 0, NUM_STEPS - 1);

    // Calculate step box width based on available space
    // 256 pixels, 8 steps max, with gaps
    int totalWidth = 252;
    int boxWidth = (totalWidth - (stepCount - 1) * UI_STEP_GAP) / stepCount;
    if (boxWidth > 30) boxWidth = 30;

    for (int s = 0; s < stepCount; s++) {
        int x = UI_LEFT_MARGIN + s * (boxWidth + UI_STEP_GAP);
        int y = UI_STEP_Y;
        int x2 = x + boxWidth;
        int y2 = y + UI_STEP_HEIGHT;

        StepState* ss = &alg->stepStates[s];
        StepParams sp = StepParams::fromAlgorithm(v, s);
        bool enabled = sp.enabled();
        bool isPlaying = transportIsRunning(dtc->transportState) && (s == dtc->currentPlayStep);
        bool isEdit = (s == editStep);

        // Background (playing step only)
        if (isPlaying) {
            NT_drawShapeI(kNT_rectangle, x, y, x2, y2, UI_BRIGHTNESS_LOW);
        }

        // Edit step outline (blinks when recording)
        if (isEdit) {
            bool recording = (v[kParamRecord] == 1);
            bool showOutline = !recording || (dtc->drawFrameCount / UI_BLINK_HALF_PERIOD) & 1;
            if (showOutline) {
                NT_drawShapeI(kNT_box, x, y, x2, y2, UI_BRIGHTNESS_MAX);
            }
        }

        // Step label (centered in box)
        int textBright = enabled ? UI_BRIGHTNESS_MAX : 2;
        int cx = x + boxWidth / 2;
        int tmpl = sp.chordTemplate();
        if (tmpl > 0) {
            static const char* const tmplAbbrev[] = {
                "", "N", "5h", "Tr", "7h", "S2", "S4", "Sh", "Q4", "Cl"
            };
            int textY = y + (UI_STEP_HEIGHT + UI_FONT_NORMAL_ASCENT) / 2;
            NT_drawText(cx, textY, tmplAbbrev[tmpl], textBright, kNT_textCentre, kNT_textNormal);
        } else if (ss->baseChord.count > 0) {
            drawDegreeGrid(x, y, boxWidth, &ss->baseChord, textBright);
        } else {
            int textY = y + (UI_STEP_HEIGHT + UI_FONT_NORMAL_ASCENT) / 2;
            NT_drawText(cx, textY, "--", textBright, kNT_textCentre, kNT_textNormal);
        }

        // Disabled indicator
        if (!enabled) {
            // Draw dim X
            NT_drawShapeI(kNT_line, x + 2, y + 2, x2 - 2, y2 - 2, 2);
            NT_drawShapeI(kNT_line, x2 - 2, y + 2, x + 2, y2 - 2, 2);
        }
    }
}

// ============================================================================
// CHORD VISUALIZATION
// ============================================================================

static void drawChordVisualization(ChordshiftAlgorithm* alg) {
    Chordshift_DTC* dtc = alg->dtc;
    // Show the most recently rendered chord
    int playStep = dtc->currentPlayStep;
    if (playStep >= NUM_STEPS) return;

    StepState* ss = &alg->stepStates[playStep];
    RenderedChord* rc = &ss->lastRendered;

    if (rc->count == 0) return;

    int y = UI_CHORD_Y;
    int barWidth = 240 / rc->count;
    if (barWidth > 28) barWidth = 28;
    if (barWidth < 4) barWidth = 4;

    for (int i = 0; i < rc->count; i++) {
        int x = UI_LEFT_MARGIN + i * (barWidth + 1);
        RenderedNote* rn = &rc->notes[i];

        // Velocity bar
        int barHeight = (rn->velocity * UI_BAR_MAX_HEIGHT) / 127;
        if (barHeight < 1) barHeight = 1;
        int barY = y + UI_BAR_MAX_HEIGHT - barHeight;

        int brightness = transportIsRunning(dtc->transportState) ? UI_BRIGHTNESS_MED : UI_BRIGHTNESS_DIM;
        NT_drawShapeI(kNT_rectangle, x, barY, x + barWidth - 1, y + UI_BAR_MAX_HEIGHT - 1, brightness);

        // Note name below bar
        if (barWidth >= 8) {
            int noteIdx = rn->midiNote % 12;
            NT_drawText(x + barWidth / 2, y + UI_BAR_MAX_HEIGHT + UI_FONT_NORMAL_ASCENT + 1,
                        noteNames[noteIdx], UI_BRIGHTNESS_MED, kNT_textCentre, kNT_textNormal);
        }
    }
}

// ============================================================================
// MAIN DRAW
// ============================================================================

bool drawUI(ChordshiftAlgorithm* alg) {
    drawStepGrid(alg);
    drawChordVisualization(alg);

    return false;  // Show standard parameter line at top
}
