#pragma once
#include "synthesis_mixing.h"
#include "synthesis_parameter.h"
#include <algae.h>
using algae::dsp::_Filter;
using algae::dsp::control::ADEnvelope;
using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Allpass2Comb;
using algae::dsp::filter::InterpolatedDelay;
using algae::dsp::filter::Onepole;
using algae::dsp::filter::Onezero;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::oscillator::SinOscillator;
using algae::dsp::oscillator::WhiteNoise;

template <typename sample_t> struct KarplusStringVoice {
  sample_t active = false;

  static const size_t MAX_DELAY_SAMPS =
      static_cast<size_t>((70.0 * 48000.0) / 1000.0);

  sample_t exciterMix = 0.5;
  sample_t allpassFilterGain = -0.5;
  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  sample_t feedback = 0.99;
  sample_t b1Coefficient = 1.0;
  sample_t h1 = 1.0;
  sample_t gain = 1.0;
  sample_t attackTime = 1;
  sample_t releaseTime = 1000;

  ASREnvelope<sample_t> env;
  WhiteNoise<sample_t> exciterNoise;
  SinOscillator<sample_t, sample_t> exciterTone;
  ADEnvelope<sample_t> exciterEnvelope;
  Allpass2Comb<sample_t, MAX_DELAY_SAMPS> apf;
  InterpolatedDelay<sample_t, MAX_DELAY_SAMPS> delay;
  Onepole<sample_t, sample_t> inputFilter;
  Onezero<sample_t> lp;
  sample_t soundSource = 0;

  sample_t sampleRate = 48000;
  sample_t y1 = 0;

  KarplusStringVoice<sample_t>() { init(); }
  KarplusStringVoice<sample_t>(const sample_t sr) : sampleRate(sr) { init(); }
  void init() {
    env.set(attackTime, releaseTime, sampleRate);
    inputFilter.lowpass(19000, sampleRate);
    exciterEnvelope.set(0.1, 1, sampleRate);
  }

  void setAllpassHarmonic(const sample_t ratio) { h1 = ratio; }

  void setSampleRate(sample_t sr) { sampleRate = sr; }

  const inline sample_t next() {

    const sample_t freq = frequency.next();
    const sample_t dtime = (sampleRate / freq);
    const sample_t allpassDelayTimeSamples = (sampleRate / (h1 * freq));

    exciterTone.setFrequency(freq, sampleRate);
    sample_t tone = exciterTone.next();
    sample_t noise = exciterNoise.next();

    sample_t exciterLevel = exciterEnvelope.next();
    // sample_t exciter = lerp<sample_t>(noise, tone, exciterLevel);
    sample_t exciter = linearXFade4<sample_t>(
        noise, exciterLevel > 0.99, exciterLevel * 2.0 - 1.0,
        algae::dsp::oscillator::SineTable<sample_t, 256>::lookup(exciterLevel),
        soundSource);
    exciter = exciter *= exciterLevel;
    exciter *= gain;

    exciter = inputFilter.next(exciter);

    sample_t s1 = inputFilter.next(exciter);
    delay.delayTimeSamples = dtime;

    sample_t s2 = delay.next(y1);
    s2 *= feedback;
    apf.g = allpassFilterGain;
    apf.delayTimeSamples = allpassDelayTimeSamples;
    s2 = apf.next(s2);
    lp.b1 = b1Coefficient;
    s2 = lp.next(s2);

    y1 = s1 + s2;

    env.set(attackTime, releaseTime, sampleRate);
    sample_t envelopeSample = env.next();
    if (env.stage == ASREnvelope<sample_t>::Stage::OFF) {
      active = false;
    }

    return y1 * gain * envelopeSample;
  }
};

template <typename sample_t> struct KarplusStrongSynthesizer {
  static const size_t MAX_VOICES = 8;
  sample_t gain = 1;
  KarplusStringVoice<sample_t> voices[MAX_VOICES];
  int notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
  size_t voiceIndex = 0;
  sample_t sampleRate = 48000;

  inline void setSampleRate(sample_t sr) {
    sampleRate = sr;
    for (auto &voice : voices) {
      voice.setSampleRate(sampleRate);
    }
  }

  KarplusStrongSynthesizer<sample_t>() { setSampleRate(sampleRate); }

  inline const sample_t next() {
    sample_t out = 0;
    for (auto &string : voices) {
      if (string.active) {
        out += string.next();
      }
    }
    return (out * gain);
  }

  inline void process(sample_t *buffer, const size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; ++i) {
      buffer[i] = next();
    }
  }

  inline void note(sample_t note, sample_t velocity) {

    if (velocity > 0) {
      SDL_Log("phys note on (%f, %f) for %d", note, velocity, voiceIndex);
      voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
      voices[voiceIndex].gain = velocity / 127.0;
      voices[voiceIndex].exciterTone.phase = 0;
      voices[voiceIndex].env.setGate(true);
      voices[voiceIndex].exciterEnvelope.setGate(true);
      voices[voiceIndex].active = true;
      notes[voiceIndex] = note;
      voiceIndex = (voiceIndex + 1) % MAX_VOICES;
    } else {
      for (size_t i = 0; i < MAX_VOICES; i++) {
        if (notes[i] == note) {
          voices[i].exciterEnvelope.setGate(false);
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
    value = clamp<sample_t>(value, 0, 1);
    value = lerp<sample_t>(0.125, 2, value);
    for (auto &voice : voices) {
      voice.h1 = value;
    }
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    value = lerp<sample_t>(-0.999, 0.999, value);
    for (auto &voice : voices) {
      voice.allpassFilterGain = value;
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
