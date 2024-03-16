#pragma once

// used by keyboard... used by step sequencer, game and touch pad.
// function takes in a float 0-1, a scale/key, and maps it to a frequency

#include <array>
#include <cstddef>
#include <math.h>
#include <string>
#include <vector>
enum class ScaleType {
  IONIAN_PENT,
  MIXOLYDIAN_PENT,
  DORIAN_PENT,
  AEOLIAN_PENT,
  PHRYGIAN_PENT,
  LYDIAN,
  IONIAN,
  MIXOLYDIAN,
  DORIAN,
  AEOLIAN,
  PHRYGIAN,
  LOCRIAN,
  HARMONIC_MAJOR,
  MELODIC_MINOR,
  HARMONIC_MINOR,
  WHOLETONE,
  OCTATONIC_1,
  OCTATONIC_2,
  CHROMATIC,
  ScaleType__SIZE
};
static const size_t NUM_SCALE_TYPES = 19;
static_assert(static_cast<size_t>(ScaleType::ScaleType__SIZE) ==
                  NUM_SCALE_TYPES,
              "there are 16 scales");
static const ScaleType ScaleTypes[NUM_SCALE_TYPES] = {
    ScaleType::IONIAN_PENT,    ScaleType::MIXOLYDIAN_PENT,
    ScaleType::DORIAN_PENT,    ScaleType::AEOLIAN_PENT,
    ScaleType::PHRYGIAN_PENT,  ScaleType::LYDIAN,
    ScaleType::IONIAN,         ScaleType::MIXOLYDIAN,
    ScaleType::DORIAN,         ScaleType::AEOLIAN,
    ScaleType::PHRYGIAN,       ScaleType::LOCRIAN,
    ScaleType::HARMONIC_MAJOR, ScaleType::MELODIC_MINOR,
    ScaleType::HARMONIC_MINOR, ScaleType::WHOLETONE,
    ScaleType::OCTATONIC_1,    ScaleType::OCTATONIC_2,
    ScaleType::CHROMATIC,
};

static const char *getDisplayName(ScaleType scaleType) {
  switch (scaleType) {

  case ScaleType::LYDIAN:
    return "lydian";
  case ScaleType::IONIAN_PENT:
    return "major pentatonic";
  case ScaleType::IONIAN:
    return "major";
  case ScaleType::MIXOLYDIAN_PENT:
    return "mixolydian pentatonic";
  case ScaleType::MIXOLYDIAN:
    return "mixolydian";
  case ScaleType::DORIAN_PENT:
    return "dorian pent";
  case ScaleType::DORIAN:
    return "dorian";
  case ScaleType::AEOLIAN_PENT:
    return "minor pentatonic";
  case ScaleType::AEOLIAN:
    return "minor";
  case ScaleType::PHRYGIAN_PENT:
    return "phrygian pentatonic";
  case ScaleType::PHRYGIAN:
    return "phrygian";
  case ScaleType::LOCRIAN:
    return "locrian";
  case ScaleType::WHOLETONE:
    return "wholetone";
  case ScaleType::OCTATONIC_1:
    return "diminished whole-half";
  case ScaleType::OCTATONIC_2:
    return "diminished half-whole";
  case ScaleType::CHROMATIC:
    return "chromatic";
  case ScaleType::HARMONIC_MAJOR:
    return "harmonic major";
  case ScaleType::MELODIC_MINOR:
    return "melodic minor";
  case ScaleType::HARMONIC_MINOR:
    return "harmonic minor";
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

static const PitchCollection &GetScale(ScaleType scaleType) {
  static const PitchCollection lydian = {
      .notes = {0, 0, 2, 2, 4, 6, 6, 7, 7, 9, 9, 11}};
  static const PitchCollection majorPent = {
      .notes = {0, 0, 2, 2, 4, 4, 4, 7, 7, 9, 9, 9}};
  static const PitchCollection major = {
      .notes = {0, 0, 2, 2, 4, 5, 5, 7, 7, 9, 9, 11}};
  static const PitchCollection harmonicMajor = {
      .notes = {0, 0, 2, 2, 4, 5, 5, 7, 7, 8, 8, 11}};
  static const PitchCollection mixolydianPent = {
      .notes = {0, 0, 2, 2, 5, 5, 5, 7, 7, 9, 9, 9}};
  static const PitchCollection mixolydian = {
      .notes = {0, 0, 2, 2, 4, 5, 7, 7, 9, 9, 10, 10}};
  static const PitchCollection melodicMinor = {
      .notes = {0, 0, 2, 2, 3, 5, 5, 7, 7, 9, 9, 11}};
  static const PitchCollection harmonicMinor = {
      .notes = {0, 0, 2, 2, 3, 5, 5, 7, 7, 8, 8, 11}};
  static const PitchCollection dorianPent = {
      .notes = {0, 0, 2, 2, 5, 5, 5, 7, 7, 10, 10, 10}};
  static const PitchCollection dorian = {
      .notes = {0, 0, 2, 2, 3, 5, 5, 7, 7, 9, 9, 10}};
  static const PitchCollection minorPent = {
      .notes = {0, 0, 3, 3, 3, 5, 7, 7, 10, 10, 10}};
  static const PitchCollection minor = {
      .notes = {0, 0, 2, 2, 3, 5, 5, 7, 7, 8, 8, 10}};
  static const PitchCollection phrygianPent = {
      .notes = {0, 0, 3, 3, 5, 5, 5, 8, 8, 10, 10, 10}};
  static const PitchCollection phrygian = {
      .notes = {0, 0, 1, 1, 3, 3, 5, 5, 7, 7, 8, 10}};
  static const PitchCollection locrian = {
      .notes = {0, 0, 1, 3, 3, 6, 6, 8, 8, 10, 10}};
  static const PitchCollection wholetone = {
      .notes = {0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10}};
  static const PitchCollection octatonic1 = {
      .notes = {0, 0, 2, 3, 3, 5, 6, 8, 8, 9, 9, 11}};
  static const PitchCollection octatonic2 = {
      .notes = {0, 1, 1, 3, 4, 4, 6, 7, 7, 9, 10, 10}};
  static const PitchCollection chromatic = {
      .notes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};

  switch (scaleType) {

  case ScaleType::LYDIAN:
    return lydian;
  case ScaleType::IONIAN_PENT:
    return majorPent;
  case ScaleType::IONIAN:
    return major;
  case ScaleType::MIXOLYDIAN_PENT:
    return mixolydianPent;
  case ScaleType::MIXOLYDIAN:
    return mixolydian;
  case ScaleType::DORIAN_PENT:
    return dorianPent;
  case ScaleType::DORIAN:
    return dorian;
  case ScaleType::AEOLIAN_PENT:
    return minorPent;
  case ScaleType::AEOLIAN:
    return minor;
  case ScaleType::PHRYGIAN_PENT:
    return phrygianPent;
  case ScaleType::PHRYGIAN:
    return phrygian;
  case ScaleType::LOCRIAN:
    return locrian;
  case ScaleType::WHOLETONE:
    return wholetone;
  case ScaleType::OCTATONIC_1:
    return octatonic1;
  case ScaleType::OCTATONIC_2:
    return octatonic2;
  case ScaleType::CHROMATIC:
    return chromatic;
  case ScaleType::HARMONIC_MAJOR:
    return harmonicMajor;
  case ScaleType::MELODIC_MINOR:
    return melodicMinor;
  case ScaleType::HARMONIC_MINOR:
    return harmonicMinor;
  case ScaleType::ScaleType__SIZE:
    break;
  }
  return chromatic;
}

inline float ForceToScale(float pitch, const PitchCollection &pitchCollection) {
  int noteIndex = pitch;
  noteIndex = noteIndex % PitchCollection::SIZE;
  const int octave = 12;
  return pitchCollection.notes[noteIndex] +
         floor(pitch / PitchCollection::SIZE) * octave;
}

inline const std::string GetNoteName(int note) {
  note = note % 12;
  switch (note) {
  case 0:
    return "c";
    break;
  case 1:
    return "c#";
    break;
  case 2:
    return "d";
    break;
  case 3:
    return "d#";
    break;
  case 4:
    return "e";
    break;
  case 5:
    return "f";
    break;
  case 6:
    return "f#";
    break;
  case 7:
    return "g";
    break;
  case 8:
    return "g#";
    break;
  case 9:
    return "a";
    break;
  case 10:
    return "a#";
    break;
  case 11:
    return "b";
    break;
  }
  return "";
}
