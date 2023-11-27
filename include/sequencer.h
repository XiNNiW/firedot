#pragma once

#include "synthesis.h"

struct Sequencer {
  Synthesizer<float> *synthesizer;
  static const size_t MAX_STEPS = 64;
  float stepValues[MAX_STEPS];
  float tempo = 160;
  float timeSinceLastStep = 0;
  float stepInterval = 1 / (4 * tempo);
  int currentStep = 0;

  void update(const float deltaTimeSeconds) {
    timeSinceLastStep += deltaTimeSeconds;
    if (timeSinceLastStep > stepInterval) {
      synthesizer->setSoundSource(stepValues[currentStep]);
      synthesizer->note(48, 120);
    } else if (timeSinceLastStep > stepInterval / 2) {
      synthesizer->note(48, 0);
    }
  }
};
