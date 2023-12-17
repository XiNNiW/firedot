#pragma once

#include "algae.h"
#include "synthesis_parameter.h"
#include <cstddef>
using algae::dsp::math::clamp;
using algae::dsp::math::lerp;

template <typename sample_t, typename DerivedT>
struct AbstractPolyphonicSynthesizer {

  static const size_t MAX_VOICES = 8;
  size_t voiceIndex = 0;
  sample_t gain = 1;
  sample_t notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
  sample_t sampleRate = 48000;

  inline const sample_t next() {
    sample_t out = 0;
    for (auto &voice : static_cast<DerivedT *>(this)->voices) {
      if (voice.active) {
        out += voice.next();
      }
    }
    return (out * gain * 0.1);
  }

  inline void process(sample_t *buffer, const size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; ++i) {
      buffer[i] = next();
    }
  }

  inline void note(sample_t frequency, sample_t gate) {
    auto voices = static_cast<DerivedT *>(this)->voices;
    if (gate > 0) {
      voices[voiceIndex].setGate(true);
      voices[voiceIndex].active = true;
      voices[voiceIndex].gain = gain;
      voices[voiceIndex].frequency.set(frequency, 5, sampleRate);
      notes[voiceIndex] = frequency;
      ++voiceIndex;
      if (voiceIndex >= MAX_VOICES)
        voiceIndex = 0;
    } else {
      for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (notes[i] == frequency) {
          voices[i].setGate(false);
          notes[i] = -1;
          break;
        }
      }
    }
  }

  inline void bendNote(const sample_t frequency,
                       const sample_t destinationFrequency) {
    for (size_t i = 0; i < MAX_VOICES; ++i) {
      if (notes[i] == frequency) {
        static_cast<DerivedT *>(this)->voices[i].frequency.set(
            destinationFrequency, 30.0, sampleRate);
      }
    }
  }

  inline void setGain(sample_t value) { gain = value; }

  inline void setFilterCutoff(sample_t value) {
    value = lerp<sample_t>(500, 19000, value);
    for (auto &voice : static_cast<DerivedT *>(this)->voices) {
      voice.filterCutoff = value;
    }
  }

  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0.001, 1);
    value *= 3;
    for (auto &voice : static_cast<DerivedT *>(this)->voices) {
      voice.filterQuality = value;
    }
  }

  inline void setSoundSource(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    for (auto &voice : static_cast<DerivedT *>(this)->voices) {
      voice.osc.oscMix = value;
    }
  }

  inline void setAttackTime(sample_t value) {
    value *= 1000;
    for (auto &voice : static_cast<DerivedT *>(this)->voices) {
      voice.attackTime = value;
    }
  }

  inline void setReleaseTime(sample_t value) {
    value *= 1000;
    for (auto &voice : static_cast<DerivedT *>(this)->voices) {
      voice.releaseTime = value;
    }
  }
};
