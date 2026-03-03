/*
 * Chordshift - Serialization
 * Save and load chord data
 */

#pragma once

#include "types.h"
#include <distingnt/serialisation.h>

void serialiseData(ChordshiftAlgorithm* alg, _NT_jsonStream& stream);
bool deserialiseData(ChordshiftAlgorithm* alg, _NT_jsonParse& parse);
