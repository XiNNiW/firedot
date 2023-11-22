
#pragma once
#include "synthesis_parameter.h"
#include <algae.h>
#include <cstddef>
#include <cstdlib>

using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Biquad;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::math::mtof;
using algae::dsp::oscillator::blep;
using algae::dsp::oscillator::computePhaseIncrement;

struct AudioSample {
  float sampleRate;
  size_t numChannels;
  size_t size;
  float *buffer;
  AudioSample(const float _sampleRate, const size_t _numChannels,
              const size_t _bufferSize)
      : sampleRate(_sampleRate), numChannels(_numChannels), size(_bufferSize),
        buffer((float *)calloc(size, sizeof(float))) {}
  ~AudioSample() {
    if (buffer) {
      // delete[] buffer;
      free(buffer);
    }
  }
};

template <typename sample_t> struct SamplerVoice {
  Parameter<sample_t> frequency;
  ASREnvelope<sample_t> env;
  Biquad<sample_t, sample_t> filter;
  sample_t *buffer = NULL;
  size_t bufferSize = 0;
  sample_t sampleRate = 48000;
  sample_t originalSampleRate = 48000;
  sample_t gain = 0;
  sample_t phase = 0;
  sample_t phaseIncrement = 0;
  sample_t attackTime = 10;
  sample_t releaseTime = 1000;
  sample_t active = 0;
  sample_t filterCutoff = 10000;
  sample_t filterQuality = 0.01;

  SamplerVoice<sample_t>() { init(); }

  inline void init() {
    filter.lowpass(filterCutoff, filterQuality, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);
  }

  inline void setSampleRate(sample_t sampleRate) {
    sampleRate = sampleRate;
    init();
  }

  inline void setFrequency(const sample_t frequency,
                           const sample_t sampleRate) {
    sample_t ratio = frequency / mtof<sample_t>(36);
    phaseIncrement = ratio * (1.0 / sample_t(bufferSize)) *
                     (sampleRate / originalSampleRate);
    // SDL_LogInfo(0, "phi %f", phaseIncrement);
  }

  inline const sample_t next() {

    auto nextFrequency = frequency.next();
    setFrequency(nextFrequency, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);
    auto envelopeSample = env.next();
    if (env.stage == ASREnvelope<sample_t>::Stage::OFF) {
      active = false;
    }

    filter.lowpass(filterCutoff, filterQuality, sampleRate);

    sample_t readPosition = phase * (bufferSize - 1);
    int r1 = floor(readPosition);
    int r2 = (r1 + 1) % bufferSize;
    sample_t mantissa = readPosition - sample_t(r1);

    sample_t out = lerp(buffer[r1], buffer[r2], mantissa);

    out = filter.next(out);
    out *= env.next();

    phase += phaseIncrement;
    phase = phase > 1 ? phase - 1 : phase;
    return out;
  }
};

template <typename sample_t> struct Sampler {
  static const size_t MAX_VOICES = 8;
  SamplerVoice<sample_t> voices[MAX_VOICES];
  int notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
  sample_t gain = 1;
  size_t voiceIndex = 0;
  sample_t sampleRate = 48000;
  sample_t *buffer;
  size_t bufferSize;

  inline void setSampleRate(sample_t sr) {
    sampleRate = sr;
    for (auto &voice : voices) {
      voice.setSampleRate(sampleRate);
    }
  }

  Sampler<sample_t>(sample_t *_buffer, const size_t &_bufferSize)
      : buffer(_buffer), bufferSize(_bufferSize) {
    for (auto &voice : voices) {
      voice.buffer = buffer;
      voice.bufferSize = bufferSize;
    }
    setSampleRate(sampleRate);
  }

  inline const sample_t next() {
    sample_t out = 0;
    for (auto &voice : voices) {
      if (voice.active) {
        out += voice.next();
      }
    }
    return out * gain;
  }

  inline void process(sample_t *buffer, const size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; ++i) {
      buffer[i] = next();
    }
  }

  inline void note(sample_t note, sample_t velocity) {

    if (velocity > 0) {
      SDL_Log("sampler note on (%f, %f) for %d", note, velocity, voiceIndex);
      voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
      voices[voiceIndex].gain = velocity / 127.0;
      voices[voiceIndex].env.setGate(true);
      voices[voiceIndex].active = true;
      voices[voiceIndex].phase = 0;
      notes[voiceIndex] = note;
      voiceIndex = (voiceIndex + 1) % MAX_VOICES;
    } else {
      for (size_t i = 0; i < MAX_VOICES; i++) {
        if (notes[i] == note) {
          voices[i].env.setGate(false);
          notes[i] = -1;
          break;
        }
      }
    }
  }
  inline void bendNote(const sample_t note, const sample_t destinationNote) {
    for (size_t i = 0; i < MAX_VOICES; ++i) {
      if (notes[i] == note) {

        voices[i].frequency.set(mtof(destinationNote), 30.0, sampleRate);
      }
    }
  }
  inline void setGain(sample_t value) { gain = value; }
  inline void setFilterCutoff(sample_t value) {
    value = lerp<sample_t>(500, 19000, value);
    for (auto &voice : voices) {
      voice.filterCutoff = value;
    }
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0.001, 1);
    value *= 3;
    for (auto &voice : voices) {
      voice.filterQuality = value;
    }
  }
  inline void setSoundSource(sample_t value) {
    for (auto &voice : voices) {
      // voice.exciterMix = value;
    }
  }
  inline void setAttackTime(sample_t value) {
    value *= 1000;
    for (auto &voice : voices) {
      voice.attackTime = value;
    }
  }
  inline void setReleaseTime(sample_t value) {
    value *= 1000;
    for (auto &voice : voices) {
      voice.releaseTime = value;
    }
  }
};
