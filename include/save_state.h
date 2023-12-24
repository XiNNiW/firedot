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
  InstrumentMetaphorType instrumentMetaphor = KEYBOARD;
  InputMapping<float> sensorMapping;
  SynthesizerSettings synthesizerSettings;
};

inline static bool SaveGame(const std::string &filename,
                            const Synthesizer<float> &synth,
                            const SaveState &state) {
  SDL_Log("save state!");
  return true;
}

inline bool LoadGame(const std::string &filename, Synthesizer<float> *synth,
                     SaveState *state) {
  SDL_Log("load state!");
  return true;
}
