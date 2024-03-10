#pragma once

#include "mapping.h"
#include "metaphor.h"
#include "save_state.h"
#include "synthesis.h"
#include <algorithm>

class Sequencer {
private:
  float tempoBPM = 98;
  float stepIntervalSeconds = 1.0 / (16.0 * tempoBPM / SECONDS_PER_MINUTE);
  int length = MAX_STEPS;
  bool running = false;

public:
  constexpr static const float SECONDS_PER_MINUTE = 60.0;
  constexpr static const float minBPM = 33;
  constexpr static const float maxBPM = 300;
  Synthesizer<float> *synth = NULL;
  SaveState *saveState = NULL;
  SDL_Thread *sequencerThread = NULL;
  static const size_t MAX_STEPS = 16;
  float stepValues[MAX_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0};

  float timeSinceLastStep = 0;
  int currentStep = 0;
  bool hadNoteOn = false;

  Sequencer(Synthesizer<float> *_synthesizer, SaveState *_saveState)
      : synth(_synthesizer), saveState(_saveState) {
    setTempo(tempoBPM);
  }

  void start() { running = true; }

  void stop() {
    saveState->sensorMapping.emitEvent(
        synth, SEQUENCER, MomentaryInputType::SEQUENCER_GATE, false);
    running = false;
  }

  void toggleRunning() {
    if (running) {
      stop();
    } else {
      start();
    }
  }

  const bool isRunning() const { return running; }

  void setTempoNormalized(float normalizedTempo) {
    auto tempoRange = maxBPM - minBPM;
    setTempo(normalizedTempo * tempoRange + minBPM);
  }

  void setTempo(float bpm) {
    tempoBPM = bpm;
    stepIntervalSeconds = 1.0 / (4.0 * tempoBPM / SECONDS_PER_MINUTE);
  }

  void setLength(int newLength) { length = std::clamp(newLength, 1, 16); }
  const int &getLength() const { return length; }
  void setLengthNormalized(float newLength) {
    length = std::clamp(static_cast<float>(newLength * MAX_STEPS), float(1.0),
                        float(16.0));
  }
  const float getLengthNormalized() const {
    return float(length) / float(MAX_STEPS);
  }

  const float &getTempo() const { return tempoBPM; }

  const float getTempoNormalized() const {
    return (tempoBPM - minBPM) / maxBPM;
  }

  void update(const float deltaTimeSeconds) {
    if (running) {
      timeSinceLastStep += deltaTimeSeconds;
      if (timeSinceLastStep >= stepIntervalSeconds) {
        if (stepValues[currentStep] > 0.0) {
          saveState->sensorMapping.emitEvent(
              synth, SEQUENCER, ContinuousInputType::SEQUENCER_STEP_LEVEL,
              stepValues[currentStep]);
          saveState->sensorMapping.emitEvent(
              synth, SEQUENCER, MomentaryInputType::SEQUENCER_GATE, true);
        }
        currentStep = (currentStep + 1) % length;
        timeSinceLastStep = 0;
        hadNoteOn = true;
      } else if (hadNoteOn &&
                 (timeSinceLastStep >= (stepIntervalSeconds / 2))) {
        saveState->sensorMapping.emitEvent(
            synth, SEQUENCER, MomentaryInputType::SEQUENCER_GATE, false);
      }
    }
  }
};
