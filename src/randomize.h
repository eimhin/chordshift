#pragma once

#include "types.h"

void randomizeSequence(uint32_t& randState, const int16_t* v,
                       uint32_t idx, uint32_t off, float& stepDuration,
                       StepState* stepStates);
