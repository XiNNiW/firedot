#pragma once

// used by keyboard... used by step sequencer, game and touch pad.
// function takes in a float 0-1, a scale/key, and maps it to a frequency

#include <vector>
struct PitchCollection {
  std::vector<float> notes;
};

inline float ForceToScale(float pitch, const PitchCollection &pitchCollection,
                          float numberOfInputRangePitches = 12) {

  return 0;
}
