/*
 * MIDI Chords - Serialization
 * Save and load chord data
 */

#pragma once

#include "types.h"
#include <distingnt/serialisation.h>

void serialiseData(MidiChordsAlgorithm* alg, _NT_jsonStream& stream);
bool deserialiseData(MidiChordsAlgorithm* alg, _NT_jsonParse& parse);
