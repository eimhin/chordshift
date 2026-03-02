/*
 * MIDI Chords - Custom UI
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

static void drawStepGrid(MidiChordsAlgorithm* alg) {
    MidiChords_DTC* dtc = alg->dtc;
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

        // Background
        int fill = 0;
        if (isPlaying) fill = UI_BRIGHTNESS_MED;
        else if (enabled && ss->baseChord.count > 0) fill = UI_BRIGHTNESS_DIM;

        if (fill > 0) {
            NT_drawShapeI(kNT_rectangle, x, y, x2, y2, fill);
        }

        // Edit step outline (blinks when recording)
        if (isEdit) {
            bool recording = (v[kParamRecord] == 1);
            bool showOutline = !recording || (dtc->drawFrameCount / UI_BLINK_HALF_PERIOD) & 1;
            if (showOutline) {
                NT_drawShapeI(kNT_box, x, y, x2, y2, UI_BRIGHTNESS_MAX);
            }
        }

        // Step number
        char label[4];
        label[0] = '1' + s;
        label[1] = '\0';
        int textBright = enabled ? UI_BRIGHTNESS_MAX : 2;
        NT_drawText(x + 2, y + 9, label, textBright, kNT_textLeft, kNT_textNormal);

        // Chord note count
        if (ss->baseChord.count > 0) {
            char countStr[4];
            NT_intToString(countStr, ss->baseChord.count);
            NT_drawText(x + boxWidth - 8, y + 9, countStr, textBright, kNT_textRight, kNT_textNormal);
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

static void drawChordVisualization(MidiChordsAlgorithm* alg) {
    MidiChords_DTC* dtc = alg->dtc;
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
        int barHeight = (rn->velocity * 28) / 127;
        if (barHeight < 1) barHeight = 1;
        int barY = y + 28 - barHeight;

        int brightness = transportIsRunning(dtc->transportState) ? UI_BRIGHTNESS_MAX : UI_BRIGHTNESS_DIM;
        NT_drawShapeI(kNT_rectangle, x, barY, x + barWidth - 1, y + 28, brightness);

        // Note name below bar
        if (barWidth >= 8) {
            int noteIdx = rn->midiNote % 12;
            NT_drawText(x + 1, y + 30, noteNames[noteIdx], UI_BRIGHTNESS_MED, kNT_textLeft, kNT_textNormal);
        }
    }
}

// ============================================================================
// MAIN DRAW
// ============================================================================

bool drawUI(MidiChordsAlgorithm* alg) {
    drawStepGrid(alg);
    drawChordVisualization(alg);

    return false;  // Show standard parameter line at top
}
