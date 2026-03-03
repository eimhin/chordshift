/*
 * Chordshift - Playback
 * Transport, step sequencing, and play modes
 */

#pragma once

#include "types.h"

// Transport control
void handleTransportStart(ChordshiftAlgorithm* alg);
void handleTransportStop(ChordshiftAlgorithm* alg);

// Process a clock tick — advance step, apply transforms, render, emit
void processClockTick(ChordshiftAlgorithm* alg);
