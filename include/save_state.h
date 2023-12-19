#pragma once

#include "metaphor.h"
#include "sensor.h"
#include "synthesis.h"
struct SynthesizerSettings {
  SynthesizerType synthType = SynthesizerType::SUBTRACTIVE;
  float gain = 1;
  float soundSource = 0;
  float filterCutoff = 1;
  float filterQuality = 0;
  float attack = 0;
  float release = 1;
};

struct SaveState {
  InstrumentMetaphorType instrumentMetaphor;
  InputMapping<float> sensorMapping;
  SynthesizerSettings synthesizerSettings;
};
