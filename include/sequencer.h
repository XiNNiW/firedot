#pragma once

#include "save_state.h"
#include "sensor.h"
#include "synthesis.h"

class Sequencer {
private:
  float tempoBPM = 98;
  float stepIntervalSeconds = 1.0 / (16.0 * tempoBPM / SECONDS_PER_MINUTE);

public:
  constexpr static const float SECONDS_PER_MINUTE = 60.0;
  constexpr static const float minBPM = 33;
  constexpr static const float maxBPM = 300;
  Synthesizer<float> *synth = NULL;
  SaveState *saveState = NULL;
  static const size_t MAX_STEPS = 16;
  float stepValues[MAX_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0};

  float timeSinceLastStep = 0;
  int currentStep = 0;
  bool running = false;
  bool hadNoteOn = false;

  Sequencer(Synthesizer<float> *_synthesizer, SaveState *_saveState)
      : synth(_synthesizer), saveState(_saveState) {
    setTempo(tempoBPM);
  }

  void setTempoNormalized(float normalizedTempo) {
    auto tempoRange = maxBPM - minBPM;
    setTempo(normalizedTempo * tempoRange + minBPM);
  }

  void setTempo(float bpm) {
    tempoBPM = bpm;
    stepIntervalSeconds = 1.0 / (4.0 * tempoBPM / SECONDS_PER_MINUTE);
  }

  inline const float getTempo() const { return tempoBPM; }

  inline const float getTempoNormalized() const {
    return (tempoBPM - minBPM) / maxBPM;
  }

  void update(const float deltaTimeSeconds) {
    if (running) {
      timeSinceLastStep += deltaTimeSeconds;
      if (timeSinceLastStep >= stepIntervalSeconds) {
        if (stepValues[currentStep] > 0.0) {
          saveState->sensorMapping.emitEvent(
              synth, ContinuousInputType::SEQUENCER_STEP_LEVEL,
              stepValues[currentStep]);
          saveState->sensorMapping.emitEvent(
              synth, MomentaryInputType::SEQUENCER_GATE, true);
        }
        currentStep = (currentStep + 1) % MAX_STEPS;
        timeSinceLastStep = 0;
        hadNoteOn = true;
      } else if (hadNoteOn &&
                 (timeSinceLastStep >= (stepIntervalSeconds / 2))) {
        saveState->sensorMapping.emitEvent(
            synth, MomentaryInputType::SEQUENCER_GATE, false);
      }
    }
  }
};
