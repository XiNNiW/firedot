#pragma once

// used by keyboard... used by step sequencer, game and touch pad.
// function takes in a float 0-1, a scale/key, and maps it to a frequency

#include <array>
#include <cstddef>
#include <math.h>
#include <vector>
enum class ScaleType {
  LYDIAN_PENT,
  LYDIAN,
  IONIAN_PENT,
  IONIAN,
  MIXOLYDIAN_PENT,
  MIXOLYDIAN,
  DORIAN_PENT,
  DORIAN,
  AEOLIAN_PENT,
  AEOLIAN,
  PHRYGIAN_PENT,
  PHRYGIAN,
  LOCRIAN_PENT,
  LOCRIAN,
  WHOLETONE,
  OCTATONIC_1,
  OCTATONIC_2,
  CHROMATIC,
  ScaleType__SIZE
};
static_assert(static_cast<size_t>(ScaleType::ScaleType__SIZE) == 18,
              "there are 18 scales");
static const ScaleType
    ScaleTypes[static_cast<size_t>(ScaleType::ScaleType__SIZE)] = {
        ScaleType::LYDIAN_PENT,     ScaleType::LYDIAN,
        ScaleType::IONIAN_PENT,     ScaleType::IONIAN,
        ScaleType::MIXOLYDIAN_PENT, ScaleType::MIXOLYDIAN,
        ScaleType::DORIAN_PENT,     ScaleType::DORIAN,
        ScaleType::AEOLIAN_PENT,    ScaleType::AEOLIAN,
        ScaleType::PHRYGIAN_PENT,   ScaleType::PHRYGIAN,
        ScaleType::LOCRIAN_PENT,    ScaleType::LOCRIAN,
        ScaleType::WHOLETONE,       ScaleType::OCTATONIC_1,
        ScaleType::OCTATONIC_2,     ScaleType::CHROMATIC,
};

static const char *getDisplayName(ScaleType scaleType) {
  switch (scaleType) {

  case ScaleType::LYDIAN_PENT:
    return "lydianPent";
    break;
  case ScaleType::LYDIAN:
    return "lydian";
    break;
  case ScaleType::IONIAN_PENT:
    return "major pentatonic";
    break;
  case ScaleType::IONIAN:
    return "major";
    break;
  case ScaleType::MIXOLYDIAN_PENT:
    return "mixolydian pentatonic";
    break;
  case ScaleType::MIXOLYDIAN:
    return "mixolydian";
    break;
  case ScaleType::DORIAN_PENT:
    return "dorian pent";
    break;
  case ScaleType::DORIAN:
    return "dorian";
    break;
  case ScaleType::AEOLIAN_PENT:
    return "minor pentatonic";
    break;
  case ScaleType::AEOLIAN:
    return "minor";
    break;
  case ScaleType::PHRYGIAN_PENT:
    return "phrygian pentatonic";
    break;
  case ScaleType::PHRYGIAN:
    return "phrygian";
    break;
  case ScaleType::LOCRIAN_PENT:
    return "locrian pentatonic";
    break;
  case ScaleType::LOCRIAN:
    return "locrian";
    break;
  case ScaleType::WHOLETONE:
    return "wholetone";
    break;
  case ScaleType::OCTATONIC_1:
    return "diminished whole-half";
    break;
  case ScaleType::OCTATONIC_2:
    return "diminished half-whole";
    break;
  case ScaleType::CHROMATIC:
    return "chromatic";
    break;
  case ScaleType::ScaleType__SIZE:
    break;
  }
  return "";
}

struct PitchCollection {
  static constexpr size_t SIZE = 12;
  float notes[SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
};
static const PitchCollection
    Scales[static_cast<size_t>(ScaleType::ScaleType__SIZE)] = {
        PitchCollection{.notes = {0, 0, 0, 2, 2, 4, 4, 6, 6, 6, 7, 7}},
        PitchCollection{.notes = {0, 0, 2, 4, 4, 6, 6, 7, 7, 9, 9, 11}},
        PitchCollection{.notes = {0, 0, 0, 2, 2, 4, 4, 4, 7, 7, 9, 9}},
        PitchCollection{.notes = {0, 0, 2, 2, 4, 4, 5, 5, 7, 7, 9, 11}},
        PitchCollection{.notes = {0, 0, 0, 2, 2, 4, 4, 7, 7, 10, 10, 10}},
        PitchCollection{.notes = {0, 0, 2, 4, 4, 5, 7, 7, 9, 9, 10, 10}},
        PitchCollection{.notes = {0, 0, 0, 2, 2, 3, 3, 7, 7, 9, 9, 9}},
        PitchCollection{.notes = {0, 0, 2, 3, 3, 5, 5, 7, 9, 9, 10, 10}},
        PitchCollection{.notes = {0, 0, 0, 1, 1, 1, 3, 3, 7, 7, 10, 10}},
        PitchCollection{.notes = {0, 0, 1, 1, 3, 3, 5, 5, 7, 7, 8, 10}},
        PitchCollection{.notes = {0, 0, 0, 1, 1, 3, 3, 6, 6, 6, 10, 10}},
        PitchCollection{.notes = {0, 0, 1, 3, 3, 6, 6, 8, 8, 10, 10}},
        PitchCollection{.notes = {0, 2, 4, 6, 8, 10}},
        PitchCollection{.notes = {0, 2, 3, 5, 6, 8, 9, 11}},
        PitchCollection{.notes = {0, 1, 3, 4, 6, 7, 9, 10}},
        PitchCollection{.notes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}}};

inline float ForceToScale(float pitch, const PitchCollection &pitchCollection) {
  int noteIndex = pitch;
  noteIndex = noteIndex % PitchCollection::SIZE;
  const int octave = 12;
  return pitchCollection.notes[noteIndex] +
         floor(pitch / PitchCollection::SIZE) * octave;
}
