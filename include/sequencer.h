#pragma once

#include "sensor.h"
#include "synthesis.h"

struct Sequencer {
  constexpr static const float SECONDS_PER_MINUTE = 60.0;
  Synthesizer<float> *synth;
  InputMapping<float> *mapping;
  static const size_t MAX_STEPS = 16;
  float stepValues[MAX_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0};
  float tempoBPM = 98;
  float timeSinceLastStep = 0;
  float stepIntervalSeconds = 1.0 / (16.0 * tempoBPM / SECONDS_PER_MINUTE);
  int currentStep = 0;
  bool running = false;
  bool hadNoteOn = false;

  Sequencer(Synthesizer<float> *_synthesizer, InputMapping<float> *_mapping)
      : synth(_synthesizer), mapping(_mapping) {}

  void update(const float deltaTimeSeconds) {

    if (running) {
      timeSinceLastStep += deltaTimeSeconds;
      if (timeSinceLastStep > stepIntervalSeconds) {
        if (stepValues[currentStep] > 0.0) {
          mapping->emitEvent(synth, ContinuousInputType::SEQUENCER_STEP_LEVEL,
                             stepValues[currentStep]);
          mapping->emitEvent(synth, MomentaryInputType::INSTRUMENT_GATE, true);
        }
        currentStep = (currentStep + 1) % MAX_STEPS;
        timeSinceLastStep = 0;
        hadNoteOn = true;
      } else if (hadNoteOn && (timeSinceLastStep > (stepIntervalSeconds / 2))) {
        mapping->emitEvent(synth, MomentaryInputType::INSTRUMENT_GATE, false);
      }
    }
  }
};
