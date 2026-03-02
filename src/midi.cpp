/*
 * MIDI Chords - MIDI Output
 *
 * Handles note emission with strum delays, note-off tracking via PlayingNote
 * array, and duration countdown for automatic note-off.
 */

#include "midi.h"
#include "math.h"
#include "midi_utils.h"

// ============================================================================
// NOTE EMISSION
// ============================================================================

// Send a single note-on immediately
static void sendNoteOn(MidiChordsAlgorithm* alg, uint8_t note, uint8_t velocity,
                       uint16_t gateMs, uint8_t outCh, uint32_t where) {
    // Release any existing note on the same pitch first
    PlayingNote* existing = &alg->playing[note];
    if (existing->active) {
        NT_sendMidi3ByteMessage(existing->where, withChannel(kMidiNoteOff, existing->outCh), note, 0);
    } else {
        alg->activeNoteCount++;
    }

    NT_sendMidi3ByteMessage(where, withChannel(kMidiNoteOn, outCh), note, velocity);
    existing->active = true;
    existing->remainingMs = gateMs;
    existing->outCh = outCh;
    existing->where = where;
}

// Schedule a note for delayed playback (strum)
static bool scheduleDelayedNote(MidiChordsAlgorithm* alg, uint8_t note, uint8_t velocity,
                                uint8_t outCh, uint16_t gateMs, uint16_t delayMs, uint32_t where) {
    for (int i = 0; i < MAX_DELAYED_NOTES; i++) {
        DelayedNote* dn = &alg->delayedNotes[i];
        if (!dn->active) {
            dn->active = true;
            dn->note = note;
            dn->velocity = velocity;
            dn->outCh = outCh;
            dn->gateMs = gateMs;
            dn->delayMs = delayMs;
            dn->where = where;
            alg->delayedNoteCount++;
            return true;
        }
    }
    return false;  // Pool full
}

void emitRenderedChord(MidiChordsAlgorithm* alg, const RenderedChord* chord) {
    const int16_t* v = alg->v;
    int outCh = v[kParamMidiOutCh];
    uint32_t where = destToWhere(v[kParamDestination]);

    for (int i = 0; i < chord->count; i++) {
        const RenderedNote* rn = &chord->notes[i];

        if (rn->delayMs == 0) {
            sendNoteOn(alg, rn->midiNote, rn->velocity, rn->gateMs, (uint8_t)outCh, where);
        } else {
            scheduleDelayedNote(alg, rn->midiNote, rn->velocity,
                              (uint8_t)outCh, rn->gateMs, rn->delayMs, where);
        }
    }
}

// ============================================================================
// ALL NOTES OFF
// ============================================================================

void sendAllNotesOff(MidiChordsAlgorithm* alg) {
    const int16_t* v = alg->v;
    int outCh = v[kParamMidiOutCh];
    uint32_t where = destToWhere(v[kParamDestination]);
    NT_sendMidi3ByteMessage(where, withChannel(kMidiCC, outCh), 123, 0);
}

void killAllPlayingNotes(MidiChordsAlgorithm* alg) {
    if (alg->activeNoteCount > 0) {
        for (int n = 0; n < 128; n++) {
            PlayingNote* pn = &alg->playing[n];
            if (pn->active) {
                NT_sendMidi3ByteMessage(pn->where, withChannel(kMidiNoteOff, pn->outCh), (uint8_t)n, 0);
                pn->active = false;
            }
        }
        alg->activeNoteCount = 0;
    }

    // Cancel pending delayed notes
    if (alg->delayedNoteCount > 0) {
        for (int i = 0; i < MAX_DELAYED_NOTES; i++) {
            alg->delayedNotes[i].active = false;
        }
        alg->delayedNoteCount = 0;
    }
}

// ============================================================================
// DELAYED NOTE PROCESSING (Strum)
// ============================================================================

void processDelayedNotes(MidiChordsAlgorithm* alg, int elapsedMs) {
    if (elapsedMs <= 0 || alg->delayedNoteCount == 0) return;

    for (int i = 0; i < MAX_DELAYED_NOTES; i++) {
        DelayedNote* dn = &alg->delayedNotes[i];
        if (!dn->active) continue;

        if (dn->delayMs <= (uint16_t)elapsedMs) {
            // Time to send
            sendNoteOn(alg, dn->note, dn->velocity, dn->gateMs, dn->outCh, dn->where);
            dn->active = false;
            if (alg->delayedNoteCount > 0) alg->delayedNoteCount--;
        } else {
            dn->delayMs -= (uint16_t)elapsedMs;
        }
    }
}

// ============================================================================
// NOTE DURATION PROCESSING
// ============================================================================

void processNoteDurations(MidiChordsAlgorithm* alg, int elapsedMs) {
    if (elapsedMs <= 0 || alg->activeNoteCount == 0) return;

    for (int n = 0; n < 128; n++) {
        PlayingNote* pn = &alg->playing[n];
        if (!pn->active) continue;

        if (pn->remainingMs <= (uint16_t)elapsedMs) {
            NT_sendMidi3ByteMessage(pn->where, withChannel(kMidiNoteOff, pn->outCh), (uint8_t)n, 0);
            pn->active = false;
            if (alg->activeNoteCount > 0) alg->activeNoteCount--;
        } else {
            pn->remainingMs -= (uint16_t)elapsedMs;
        }
    }
}
