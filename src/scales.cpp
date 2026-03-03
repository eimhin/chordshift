/*
 * Chordshift - Scale Data
 * Scale interval tables and lookup data (single definition)
 */

#include "scales.h"

const int scaleIntervals[SCALE_COUNT][7] = {
    { 0, 2, 4, 5, 7, 9, 11 },  // Ionian (Major)
    { 0, 2, 3, 5, 7, 9, 10 },  // Dorian
    { 0, 1, 3, 5, 7, 8, 10 },  // Phrygian
    { 0, 2, 4, 6, 7, 9, 11 },  // Lydian
    { 0, 2, 4, 5, 7, 9, 10 },  // Mixolydian
    { 0, 2, 3, 5, 7, 8, 10 },  // Aeolian (Natural Minor)
    { 0, 1, 3, 5, 6, 8, 10 },  // Locrian
    { 0, 2, 3, 5, 7, 8, 11 },  // Harmonic Minor
    { 0, 2, 3, 5, 7, 9, 11 },  // Melodic Minor
    { 0, 2, 4, 7, 9, 0, 0 },   // Major Pentatonic (5 notes)
    { 0, 3, 5, 7, 10, 0, 0 },  // Minor Pentatonic (5 notes)
};

const int scaleSizes[SCALE_COUNT] = {
    7, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5
};

const int PC_TO_WHITE_KEY[12] = {
    0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6
};
