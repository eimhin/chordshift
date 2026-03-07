/*
 * Chordshift - Custom UI
 *
 * 256x64 pixel display:
 * - Row 0-8: Standard param line (returned by draw() returning false)
 * - Row 10-62: Step grid
 */

#include "ui.h"
#include "math.h"
#include "scales.h"

// ============================================================================
// STEP GRID
// ============================================================================

static void drawDegreeGrid(int bx, int by, int boxWidth, const ChordDegrees* chord, int brightness) {
    static constexpr int SQ = 4;   // square size
    static constexpr int GAP = 2;  // gap between squares
    static constexpr int V_MAX_COLS = 2; // max columns in vertical layout

    // Find degree range
    int lo = chord->degrees[0], hi = chord->degrees[0];
    for (int i = 1; i < chord->count; i++) {
        if (chord->degrees[i] < lo) lo = chord->degrees[i];
        if (chord->degrees[i] > hi) hi = chord->degrees[i];
    }
    int span = hi - lo + 1;
    if (span > 15) span = 15;

    // Build presence mask
    bool present[15] = {};
    for (int i = 0; i < chord->count; i++) {
        int idx = chord->degrees[i] - lo;
        if (idx >= 0 && idx < 15)
            present[idx] = true;
    }

    // Calculate grid dimensions for centering
    static constexpr int MAX_ROWS = 7;
    static constexpr int H_THRESHOLD = MAX_ROWS * SQ + (MAX_ROWS - 1) * GAP + 14;
    bool horizontal = (boxWidth >= H_THRESHOLD);

    int rows, cols;
    if (horizontal) {
        int maxCols = (boxWidth - 6 + GAP) / (SQ + GAP);
        if (maxCols > MAX_ROWS) maxCols = MAX_ROWS;
        cols = (span < maxCols) ? span : maxCols;
        rows = (span + cols - 1) / cols;
    } else {
        cols = (span <= MAX_ROWS) ? 1 : V_MAX_COLS;
        rows = (cols == 1) ? span : MAX_ROWS;
    }
    int gridW = cols * SQ + (cols - 1) * GAP;
    int gridH = rows * SQ + (rows - 1) * GAP;
    int ox = bx + (boxWidth + 1 - gridW) / 2;
    int oy = by + (UI_STEP_HEIGHT - gridH) / 2;

    for (int i = 0; i < span; i++) {
        int col = horizontal ? (i % cols) : (i / rows);
        int row = horizontal ? (i / cols) : (i % rows);
        int x = ox + col * (SQ + GAP);
        int y = oy + row * (SQ + GAP);
        if (present[i]) {
            NT_drawShapeI(kNT_rectangle, x, y, x + SQ - 1, y + SQ - 1, brightness);
        } else {
            NT_drawShapeI(kNT_box, x, y, x + SQ - 1, y + SQ - 1, 1);
        }
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
        int cx = x + (boxWidth + 1) / 2;
        int tmpl = sp.chordTemplate();
        if (tmpl > 0) {
            const ChordTemplate& t = CHORD_TEMPLATES[tmpl];
            ChordDegrees tmplChord;
            tmplChord.count = t.count;
            for (int i = 0; i < t.count; i++)
                tmplChord.degrees[i] = t.degrees[i];
            drawDegreeGrid(x, y, boxWidth, &tmplChord, textBright);
        } else if (ss->baseChord.count > 0) {
            drawDegreeGrid(x, y, boxWidth, &ss->baseChord, textBright);
        } else {
            int textY = y + (UI_STEP_HEIGHT + UI_FONT_NORMAL_ASCENT) / 2;
            NT_drawText(cx, textY, "--", UI_BRIGHTNESS_LOW, kNT_textCentre, kNT_textNormal);
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
// MAIN DRAW
// ============================================================================

bool drawUI(ChordshiftAlgorithm* alg) {
    drawStepGrid(alg);

    return false;  // Show standard parameter line at top
}
