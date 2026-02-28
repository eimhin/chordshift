/*
 * MIDI Chords - Capture Pipeline
 * MIDI input -> degree-space chord capture
 */

#pragma once

#include "types.h"

// Called from midiMessage() for note on/off events
void captureNoteOn(MidiChordsAlgorithm* alg, uint8_t note, uint8_t velocity);
void captureNoteOff(MidiChordsAlgorithm* alg, uint8_t note);
