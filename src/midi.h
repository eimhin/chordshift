/*
 * MIDI Chords - MIDI Output
 * Note emission, note-off tracking, delayed note pool for strum
 */

#pragma once

#include "types.h"

// Emit all notes in a rendered chord (schedules strum delays as needed)
void emitRenderedChord(MidiChordsAlgorithm* alg, const RenderedChord* chord);

// Send note-off for all playing notes
void sendAllNotesOff(MidiChordsAlgorithm* alg);

// Kill all playing notes immediately
void killAllPlayingNotes(MidiChordsAlgorithm* alg);

// Process delayed notes (strum) — call from step()
void processDelayedNotes(MidiChordsAlgorithm* alg, float dt);

// Process note durations — send note-offs when gate expires
void processNoteDurations(MidiChordsAlgorithm* alg, float dt);
