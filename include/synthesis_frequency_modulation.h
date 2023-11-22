#pragma once

#include "SDL_log.h"
#include "synthesis_mixing.h"
#include "synthesis_parameter.h"
#include "synthesis_sampling.h"
#include <algae.h>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <rigtorp/SPSCQueue.h>

using algae::dsp::_Filter;
using algae::dsp::_Generator;
using algae::dsp::control::ADEnvelope;
using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Allpass2Comb;
using algae::dsp::filter::Biquad;
using algae::dsp::filter::InterpolatedDelay;
using algae::dsp::filter::Onepole;
using algae::dsp::filter::ResonantBandpass2ndOrderIIR;
using algae::dsp::filter::SmoothParameter;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::math::mtof;
using algae::dsp::oscillator::blep;
using algae::dsp::oscillator::computePhaseIncrement;
using algae::dsp::oscillator::PolyBLEPSaw;
using algae::dsp::oscillator::PolyBLEPSquare;
using algae::dsp::oscillator::PolyBLEPTri;
using algae::dsp::oscillator::SinOscillator;
using algae::dsp::oscillator::WhiteNoise;

template <typename sample_t> struct FMOperator {
  sample_t sampleRate = 48000;
  SinOscillator<sample_t, sample_t, 1024> osc;
  ASREnvelope<sample_t> env;
  sample_t freq = 440;
  sample_t last = 0;

  FMOperator<sample_t>() { env.set(10, 1000, sampleRate); }

  inline const sample_t next(sample_t pmod = 0) {
    osc.setFrequency(freq, sampleRate);
    last = env.next() * osc.next(pmod);
    return last;
  }

  inline void setFrequency(sample_t f) { freq = f; }
};

template <typename sample_t> struct FMDrumOperator {
  sample_t sampleRate = 48000;
  SinOscillator<sample_t, sample_t, 1024> osc;
  ADEnvelope<sample_t> env;
  sample_t freq = 440;
  sample_t last = 0;

  FMDrumOperator<sample_t>() { env.set(10, 1000, sampleRate); }

  inline const sample_t next(sample_t pmod = 0) {
    osc.setFrequency(freq, sampleRate);
    auto envelopeSample = env.next();
    envelopeSample *= envelopeSample;
    envelopeSample *= envelopeSample;
    last = envelopeSample * osc.next(pmod);
    return last;
  }

  inline void setFrequency(sample_t f) { freq = f; }
};

template <typename sample_t> struct FMDrumVoice {
  WhiteNoise<sample_t> noise;
  FMDrumOperator<sample_t> op1;
  FMDrumOperator<sample_t> op2;
  ADEnvelope<sample_t> env;
  sample_t active = false;
  sample_t index = 1;
  sample_t noiseModDepth = 1;
  sample_t gain = 1;
  sample_t soundSource = 0;
  sample_t filterCutoff = 0;
  sample_t filterQuality = 0;
  sample_t attackTime = 0.1;
  sample_t releaseTime = 0;

  FMDrumVoice<sample_t>() {}

  inline const sample_t next() {
    if (env.stage == ADEnvelope<sample_t>::OFF) {
      active = false;
    }
    return op1.next((op1.next() + noise.next() * noiseModDepth) * index) *
           env.next();
  }
};

template <typename sample_t> struct FMSynthesizer {
  static const size_t MAX_VOICES = 8;
  size_t voiceIndex = 0;

  FMDrumVoice<sample_t> voices[MAX_VOICES];
  sample_t gain = 1;
  sample_t notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
  sample_t sampleRate = 48000;

  inline const sample_t next() {
    sample_t out = 0;
    for (auto &voice : voices) {
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

  inline void note(const sample_t note, const sample_t velocity) {
    if (velocity > 0) {
      SDL_Log("fm note on (%f, %f) for %d", note, velocity, voiceIndex);
      voices[voiceIndex].setGate(true);
      voices[voiceIndex].active = true;
      voices[voiceIndex].gain = (velocity / 127.0);
      voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
      notes[voiceIndex] = note;
      ++voiceIndex;
      if (voiceIndex >= MAX_VOICES)
        voiceIndex = 0;
    } else {
      for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (notes[i] == note) {
          voices[i].setGate(false);
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
    value = clamp<sample_t>(value, 0, 1);
    value *= value;
    value *= value;
    //  auto fb = lerp<sample_t>(0.0, 1.0, pow(1, value));
    for (auto &voice : voices) {
      voice.index = value;
    }
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    int index = value * (FM4OpVoice<sample_t>::NUM_RATIOS - 1);

    for (auto &voice : voices) {
      voice.activeRatio = index;
    }
  }
  inline void setSoundSource(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    sample_t index = value * (FM4OpVoice<sample_t>::NUM_ALGS - 1);
    for (auto &voice : voices) {
      voice.activeTopology = index;
    }
  }
  inline void setAttackTime(sample_t value) {
    value *= 1000;
    for (auto &voice : voices) {
      voice.op1.env.setAttackTime(value, sampleRate);
      voice.op2.env.setAttackTime(value, sampleRate);
      voice.op3.env.setAttackTime(value, sampleRate);
      voice.op4.env.setAttackTime(value, sampleRate);
    }
  }
  inline void setReleaseTime(sample_t value) {
    value *= 1000;
    for (auto &voice : voices) {
      voice.op1.env.setReleaseTime(value, sampleRate);
      voice.op2.env.setReleaseTime(value, sampleRate);
      voice.op3.env.setReleaseTime(value, sampleRate);
      voice.op4.env.setReleaseTime(value, sampleRate);
    }
  }
};
template <typename sample_t> struct FM4OpVoice {

  static const size_t NUM_RATIOS = 8;
  sample_t ratios[NUM_RATIOS][4] = {{1.0, 1.0 / 64.0, 2.0, 1.0 / 16.0},
                                    {1.0, 3.0 / 32.0, 1.5, 2.0},
                                    {1.0, 1.0 / 8.0, 1.0, 5.0 / 8.0},
                                    {1.0, 5.0 / 16.0, 1.0 / 2.0, 4.0},
                                    {1.0, 0.5, 3.0 / 4.0, 1.75},
                                    {1.0, 5.0 / 8.0, 1.0 / 4.0, 7.0 / 16.0},
                                    {1.0, 1.0, 5.0 / 32.0, 3.5},
                                    {1.0, 2.0, 1.0 / 32.0, 1.0}};

  static const size_t NUM_ALGS = 8;
  sample_t topologies[NUM_ALGS][4][4] = {
      {{0, 1, 1, 0}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 0}},
      {{0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}},
      {{0, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
      {{0, 0, 1, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
      {{0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 0, 0}},
      {{0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}},
      {{0, 1, 0, 0}, {0, 0, 1, 1}, {0, 0, 0, 1}, {0, 0, 0, 0}},
      {{0, 1, 1, 1}, {0, 0, 1, 1}, {0, 0, 0, 1}, {0, 0, 0, 1}},
  };
  sample_t gainSettings[NUM_ALGS][4] = {
      {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0},
      {1, 1, 1, 0}, {1, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 0, 0},
  };

  sample_t activeTopology = 0;
  sample_t activeRatio = 0;

  sample_t index = 0.1;

  Parameter<sample_t> frequency = Parameter<sample_t>(440.0);
  sample_t gain = 1.0;
  FMOperator<sample_t> op1;
  FMOperator<sample_t> op2;
  FMOperator<sample_t> op3;
  FMOperator<sample_t> op4;
  sample_t active = false;

  inline const sample_t next() {

    auto nextFrequency = frequency.next();

    int ratioIndex = floor(activeRatio);

    op1.setFrequency(nextFrequency * ratios[ratioIndex][0]);
    op2.setFrequency(nextFrequency * ratios[ratioIndex][1]);
    op3.setFrequency(nextFrequency * ratios[ratioIndex][2]);
    op4.setFrequency(nextFrequency * ratios[ratioIndex][3]);

    int topologyIndex = floor(activeTopology);
    int nextTopologyIndex = (topologyIndex + 1) % NUM_ALGS;
    sample_t topologyMantissa = activeTopology - sample_t(topologyIndex);

    sample_t mod1 = index * (op1.last * topologies[topologyIndex][0][0] +
                             op2.last * topologies[topologyIndex][0][1] +
                             op3.last * topologies[topologyIndex][0][2] +
                             op4.last * topologies[topologyIndex][0][3]);
    sample_t mod2 = index * (op1.last * topologies[topologyIndex][1][0] +
                             op2.last * topologies[topologyIndex][1][1] +
                             op3.last * topologies[topologyIndex][1][2] +
                             op4.last * topologies[topologyIndex][1][3]);
    sample_t mod3 = index * (op1.last * topologies[topologyIndex][2][0] +
                             op2.last * topologies[topologyIndex][2][1] +
                             op3.last * topologies[topologyIndex][2][2] +
                             op4.last * topologies[topologyIndex][2][3]);
    sample_t mod4 = index * (op1.last * topologies[topologyIndex][3][0] +
                             op2.last * topologies[topologyIndex][3][1] +
                             op3.last * topologies[topologyIndex][3][2] +
                             op4.last * topologies[topologyIndex][3][3]);
    sample_t out = op1.next(mod1) * lerp(gainSettings[topologyIndex][0],
                                         gainSettings[nextTopologyIndex][0],
                                         topologyMantissa) +
                   op2.next(mod2) * lerp(gainSettings[topologyIndex][1],
                                         gainSettings[nextTopologyIndex][1],
                                         topologyMantissa) +
                   op2.next(mod3) * lerp(gainSettings[topologyIndex][2],
                                         gainSettings[nextTopologyIndex][2],
                                         topologyMantissa) +
                   op4.next(mod4) * lerp(gainSettings[topologyIndex][3],
                                         gainSettings[nextTopologyIndex][3],
                                         topologyMantissa);
    out *= gain;
    active = (op1.env.stage != ASREnvelope<sample_t>::OFF) ||
             (op2.env.stage != ASREnvelope<sample_t>::OFF) ||
             (op3.env.stage != ASREnvelope<sample_t>::OFF) ||
             (op4.env.stage != ASREnvelope<sample_t>::OFF);
    return out;
  }

  inline void setGate(sample_t gate) {
    op1.env.setGate(gate);
    op2.env.setGate(gate);
    op3.env.setGate(gate);
    op4.env.setGate(gate);
  }
};
template <typename sample_t> struct FMSynthesizer {
  static const size_t MAX_VOICES = 8;
  size_t voiceIndex = 0;

  FM4OpVoice<sample_t> voices[MAX_VOICES];
  sample_t gain = 1;
  sample_t notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
  sample_t sampleRate = 48000;

  inline const sample_t next() {
    sample_t out = 0;
    for (auto &voice : voices) {
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

  inline void note(const sample_t note, const sample_t velocity) {
    if (velocity > 0) {
      SDL_Log("fm note on (%f, %f) for %d", note, velocity, voiceIndex);
      voices[voiceIndex].setGate(true);
      voices[voiceIndex].active = true;
      voices[voiceIndex].gain = (velocity / 127.0);
      voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
      notes[voiceIndex] = note;
      ++voiceIndex;
      if (voiceIndex >= MAX_VOICES)
        voiceIndex = 0;
    } else {
      for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (notes[i] == note) {
          voices[i].setGate(false);
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
    value = clamp<sample_t>(value, 0, 1);
    value *= value;
    value *= value;
    //  auto fb = lerp<sample_t>(0.0, 1.0, pow(1, value));
    for (auto &voice : voices) {
      voice.index = value;
    }
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    int index = value * (FM4OpVoice<sample_t>::NUM_RATIOS - 1);

    for (auto &voice : voices) {
      voice.activeRatio = index;
    }
  }
  inline void setSoundSource(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    sample_t index = value * (FM4OpVoice<sample_t>::NUM_ALGS - 1);
    for (auto &voice : voices) {
      voice.activeTopology = index;
    }
  }
  inline void setAttackTime(sample_t value) {
    value *= 1000;
    for (auto &voice : voices) {
      voice.op1.env.setAttackTime(value, sampleRate);
      voice.op2.env.setAttackTime(value, sampleRate);
      voice.op3.env.setAttackTime(value, sampleRate);
      voice.op4.env.setAttackTime(value, sampleRate);
    }
  }
  inline void setReleaseTime(sample_t value) {
    value *= 1000;
    for (auto &voice : voices) {
      voice.op1.env.setReleaseTime(value, sampleRate);
      voice.op2.env.setReleaseTime(value, sampleRate);
      voice.op3.env.setReleaseTime(value, sampleRate);
      voice.op4.env.setReleaseTime(value, sampleRate);
    }
  }
};
