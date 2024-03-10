#pragma once

#include "synthesis_type.h"
struct SynthesizerSettings {
  SynthesizerType synthType = SynthesizerType::SUBTRACTIVE_DRUM_SYNTH;
  float gain = 1;
  float soundSource = 0;
  float filterCutoff = 1;
  float filterQuality = 0;
  float attack = 0;
  float release = 1;
  float octave = 0.5;
};
