#pragma once
#include "algae.h"

using algae::dsp::oscillator::SineTable;

template <typename sample_t> struct ClapEnvelope {
  enum Stage { ATTACK, RELEASE, OFF } stage = OFF;
  sample_t attack_increment = 4800;
  sample_t decay_increment = 48000;
  sample_t clapDensity = 4;
  sample_t lastGate = 0;
  sample_t nextOutput = 0;
  sample_t phase = 0;

  inline const sample_t next() {

    auto out = nextOutput;
    switch (stage) {
    case Stage::ATTACK: {
      phase += attack_increment;
      nextOutput = 1;
      nextOutput *=
          fabs(SineTable<sample_t, 256>::lookup(phase * clapDensity + 0.25));
      if (phase > 1) {
        stage = Stage::RELEASE;
        phase = 1;
      }
      break;
    }
    case Stage::RELEASE: {
      phase -= decay_increment;
      nextOutput = phase;
      if (phase < 0) {
        nextOutput = out = phase = 0;
        stage = Stage::OFF;
      }
      break;
    }
    case Stage::OFF: {
      break;
    }
    }

    return out;
  }

  void set(const sample_t attack_ms, const sample_t decay_ms,
           const sample_t samplerate) {
    sample_t attack_in_samples = (attack_ms * samplerate) / 1000.0;
    sample_t decay_in_samples = (decay_ms * samplerate) / 1000.0;
    if (attack_in_samples <= 0) {
      attack_in_samples = 1;
    }
    if (decay_in_samples <= 0) {
      decay_in_samples = 1;
    }
    attack_increment = 1.0 / attack_in_samples;
    decay_increment = 1.0 / decay_in_samples;
  }

  inline void setGate(const bool gate) {
    switch (stage) {
    case Stage::OFF: {
      if (gate && !lastGate) {
        phase = 0;
        nextOutput = 0;
        stage = Stage::ATTACK;
      }
      break;
    }
    case Stage::ATTACK: {
      if (!gate) {

        stage = Stage::RELEASE;
      }
      break;
    }
    case Stage::RELEASE: {
      if (gate && !lastGate) {
        stage = Stage::ATTACK;
      }
      break;
    }
    }
    lastGate = gate;
  }
};
