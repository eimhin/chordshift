/*
 * MIDI Chords - Serialization
 *
 * JSON format:
 * {
 *   "version": 1,
 *   "steps": [
 *     {"chord": [0, 2, 4, 6]},
 *     {"chord": [0, 3, 5]},
 *     ...
 *   ]
 * }
 *
 * Only baseChord data needs persistence — all transform parameters are
 * handled by the disting NT parameter system.
 */

#include "serial.h"
#include "math.h"

static const int SERIAL_VERSION = 1;

// ============================================================================
// SERIALIZATION
// ============================================================================

void serialiseData(MidiChordsAlgorithm* alg, _NT_jsonStream& stream) {
    stream.addMemberName("version");
    stream.addNumber(SERIAL_VERSION);

    stream.addMemberName("steps");
    stream.openArray();
    for (int s = 0; s < NUM_STEPS; s++) {
        stream.openObject();

        StepState* ss = &alg->stepStates[s];
        stream.addMemberName("chord");
        stream.openArray();
        for (int n = 0; n < ss->baseChord.count && n < MAX_CHORD_NOTES; n++) {
            stream.addNumber((int)ss->baseChord.degrees[n]);
        }
        stream.closeArray();

        stream.closeObject();
    }
    stream.closeArray();
}

// ============================================================================
// DESERIALIZATION
// ============================================================================

static bool parseStepObject(_NT_jsonParse& parse, StepState* ss) {
    int numMembers;
    if (!parse.numberOfObjectMembers(numMembers)) return false;

    for (int i = 0; i < numMembers; i++) {
        if (parse.matchName("chord")) {
            int numNotes;
            if (!parse.numberOfArrayElements(numNotes)) return false;

            ss->baseChord.count = 0;
            for (int n = 0; n < numNotes; n++) {
                int degree;
                if (!parse.number(degree)) return false;
                if (n < MAX_CHORD_NOTES) {
                    ss->baseChord.degrees[n] = (int8_t)clamp(degree, -127, 127);
                    ss->baseChord.count++;
                }
            }
        } else {
            if (!parse.skipMember()) return false;
        }
    }
    return true;
}

bool deserialiseData(MidiChordsAlgorithm* alg, _NT_jsonParse& parse) {
    int numMembers;
    if (!parse.numberOfObjectMembers(numMembers)) return false;

    for (int m = 0; m < numMembers; m++) {
        if (parse.matchName("version")) {
            int version;
            if (!parse.number(version)) return false;
            if (version > SERIAL_VERSION) return false;
        } else if (parse.matchName("steps")) {
            int numSteps;
            if (!parse.numberOfArrayElements(numSteps)) return false;

            for (int s = 0; s < numSteps; s++) {
                if (s < NUM_STEPS) {
                    if (!parseStepObject(parse, &alg->stepStates[s])) return false;
                    alg->stepStates[s].lastRendered.count = 0;
                } else {
                    // Skip excess steps
                    int numSkipMembers;
                    if (!parse.numberOfObjectMembers(numSkipMembers)) return false;
                    for (int i = 0; i < numSkipMembers; i++) {
                        if (!parse.skipMember()) return false;
                    }
                }
            }
            // Clear steps not present in the file
            for (int s = numSteps; s < NUM_STEPS; s++) {
                alg->stepStates[s].baseChord.count = 0;
                alg->stepStates[s].lastRendered.count = 0;
            }
        } else {
            if (!parse.skipMember()) return false;
        }
    }

    return true;
}
