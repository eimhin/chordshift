/*
 * MIDI Chords - Playback
 * Transport, step sequencing, and play modes
 */

#pragma once

#include "types.h"

// Transport control
void handleTransportStart(MidiChordsAlgorithm* alg);
void handleTransportStop(MidiChordsAlgorithm* alg);

// Process a clock tick — advance step, apply transforms, render, emit
void processClockTick(MidiChordsAlgorithm* alg);
