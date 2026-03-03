/*
 * Chordshift - MIDI Output
 * Note emission, note-off tracking, delayed note pool for strum
 */

#pragma once

#include "types.h"

// Emit all notes in a rendered chord (schedules strum delays as needed)
void emitRenderedChord(ChordshiftAlgorithm* alg, const RenderedChord* chord);

// Send note-off for all playing notes
void sendAllNotesOff(ChordshiftAlgorithm* alg);

// Kill all playing notes immediately
void killAllPlayingNotes(ChordshiftAlgorithm* alg);

// Process delayed notes (strum) — call from step()
void processDelayedNotes(ChordshiftAlgorithm* alg, int elapsedMs);

// Process note durations — send note-offs when gate expires
void processNoteDurations(ChordshiftAlgorithm* alg, int elapsedMs);
