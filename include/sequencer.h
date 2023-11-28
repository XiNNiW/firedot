#pragma once

#include "synthesis.h"

struct Sequencer {
  constexpr static const float SECONDS_PER_MINUTE = 60.0;
  Synthesizer<float> *synthesizer;
  static const size_t MAX_STEPS = 16;
  float stepValues[MAX_STEPS];
  float tempoBPM = 160;
  float timeSinceLastStep = 0;
  float stepIntervalSeconds = 1.0 / (16.0 * tempoBPM / SECONDS_PER_MINUTE);
  int currentStep = 0;
  bool running = false;

  Sequencer(Synthesizer<float> *_synthesizer) : synthesizer(_synthesizer) {}

  void update(const float deltaTimeSeconds) {

    if (running) {
      timeSinceLastStep += deltaTimeSeconds;
      if (timeSinceLastStep > stepIntervalSeconds) {
        synthesizer->setSoundSource(stepValues[currentStep]);
        if (stepValues[currentStep] > 0) {
          synthesizer->note(48, 120);
        }
        currentStep = (currentStep + 1) % MAX_STEPS;
        timeSinceLastStep = 0;
      } else if (timeSinceLastStep > stepIntervalSeconds / 2) {
        synthesizer->note(48, 0);
      }
    }
  }
};
