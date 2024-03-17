
#pragma once
// #include "SDL_audio.h"
#include "SDL_log.h"
#include "arena.h"
#include "sample_bank.h"
#include "sample_load.h"
#include "synthesis_abstract.h"
#include "synthesis_parameter.h"
#include <algae.h>
#include <vector>

using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Biquad;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::math::mtof;
using algae::dsp::oscillator::blep;
using algae::dsp::oscillator::computePhaseIncrement;

template <typename sample_t> struct SamplerVoice {
  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  ASREnvelope<sample_t> env;
  Biquad<sample_t, sample_t> filter;
  // sample_t *buffer = NULL;
  // size_t bufferSize = 0;
  SampleBank<sample_t> *sampleBank = NULL;
  sample_t sampleRate = 48000;
  sample_t originalSampleRate = 48000;
  sample_t gain = 0;
  sample_t phases[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  sample_t phaseIncrements[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0};
  sample_t soundSource = 0;
  sample_t attackTime = 10;
  sample_t releaseTime = 1000;
  Parameter<sample_t> filterCutoff = Parameter<sample_t>(10000);
  Parameter<sample_t> filterQuality = Parameter<sample_t>(0.01);

  SamplerVoice<sample_t>(SampleBank<sample_t> *bank) : sampleBank(bank) {
    init();
  }

  inline void init() {
    filter.lowpass(filterCutoff.value, filterQuality.value, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);
  }

  inline void setSampleRate(sample_t sampleRate) {
    this->sampleRate = sampleRate;
    init();
  }

  inline void setFrequency(const sample_t frequency,
                           const sample_t sampleRate) {

    for (size_t i = 0; i < sampleBank->size; i++) {
      auto bufferSize = sampleBank->buffers[i]->bufferSize;
      sample_t ratio = frequency / mtof<sample_t>(36);
      phaseIncrements[i] = ratio * (1.0 / sample_t(bufferSize)) *
                           (sampleRate / originalSampleRate);
    }
  }

  inline void setGate(sample_t gate) {
    env.setGate(gate);
    if (gate) {
      for (size_t i = 0; i < sampleBank->size; ++i) {
        phases[i] = 0;
      }
    }
  }

  inline const sample_t next() {

    auto nextFrequency = frequency.next();
    setFrequency(nextFrequency, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);

    auto envelopeSample = env.next();

    filter.lowpass(filterCutoff.next(), filterQuality.next(), sampleRate);

    sample_t sampleIndex =
        soundSource * static_cast<sample_t>(sampleBank->size - 1);
    int s1 = floor(sampleIndex);
    int s2 = (s1 + 1) % sampleBank->size;
    sample_t mantissa = sampleIndex - static_cast<sample_t>(s1);

    auto out = lerp(sampleBank->buffers[s1]->read(phases[s1]),
                    sampleBank->buffers[s2]->read(phases[s2]), mantissa);

    out = filter.next(out);
    out *= env.next() * 4;
    for (size_t i = 0; i < sampleBank->size; ++i) {
      phases[i] += phaseIncrements[i];
      phases[i] = phases[i] > 1 ? 1 : phases[i];
    }

    return out;
  }
};

template <typename sample_t>
struct Sampler : AbstractMonophonicSynthesizer<sample_t, Sampler<sample_t>> {
  sample_t *buffer;
  size_t bufferSize;
  SamplerVoice<sample_t> voice;

  inline void setSampleRate(sample_t sr) {
    this->sampleRate = sr;
    voice.setSampleRate(this->sampleRate);
  }

  inline void setSoundSource(sample_t value) { voice.soundSource = value; }
  Sampler<sample_t>(SampleBank<sample_t> *bank) : voice(bank) {
    setSampleRate(this->sampleRate);
  }
};
