#pragma once

#include "synthesis_abstract.h"
#include "synthesis_clap_envelope.h"
#include "synthesis_mixing.h"
#include "synthesis_parameter.h"
#include <algae.h>

using algae::dsp::_Filter;
using algae::dsp::control::ADEnvelope;
using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Allpass2Comb;
using algae::dsp::filter::Biquad;
using algae::dsp::filter::InterpolatedDelay;
using algae::dsp::filter::Onepole;
using algae::dsp::filter::Onezero;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::oscillator::SinOscillator;
using algae::dsp::oscillator::WhiteNoise;

template <typename sample_t>
inline const sample_t tau2pole(sample_t tau, sample_t sampleRate) {
  tau = fmax(std::numeric_limits<sample_t>::epsilon(), fabs(tau));
  return exp(-1.0 / (tau * sampleRate));
}

template <typename sample_t> struct EnvelopeFollower {
  sample_t pole = tau2pole(0.01, 48000.0);
  sample_t y1 = 0;

  inline void setReleaseTime(sample_t rel, sample_t sampleRate) {
    pole = tau2pole(rel, sampleRate);
  }

  inline const sample_t next(sample_t x) {
    y1 = fmax(x, (x * (1.0 - pole)) + y1 * pole);
    return y1;
  }
};

template <typename sample_t> struct KarplusDrumVoice {
  sample_t active = false;

  static const size_t MAX_DELAY_SAMPS =
      static_cast<size_t>((70.0 * 48000.0) / 1000.0);

  sample_t soundSource = 0;
  sample_t allpassFilterGain = -0.5;
  sample_t feedback = 0.85;
  sample_t b1Coefficient = 0.01;
  sample_t h1 = 1.73;
  sample_t gain = 1;
  sample_t attackTime = 1;
  sample_t releaseTime = 1000;
  sample_t pitchBendDepth = 50;

  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  ADEnvelope<sample_t> env;
  ADEnvelope<sample_t> ramp;
  ADEnvelope<sample_t> pitchEnv;
  WhiteNoise<sample_t> exciterNoise;
  ClapEnvelope<sample_t> exciterEnvelope;
  Allpass2Comb<sample_t, MAX_DELAY_SAMPS> apf;
  InterpolatedDelay<sample_t, MAX_DELAY_SAMPS> delay;
  Onepole<sample_t, sample_t> inputFilter;
  Biquad<sample_t, sample_t> snareFilter1;
  Biquad<sample_t, sample_t> snareFilter2;
  EnvelopeFollower<sample_t> snareFollower;
  Onepole<sample_t, sample_t> lp;
  Onepole<sample_t, sample_t> hp;
  Onepole<sample_t, sample_t> exciterHighPass;
  Onepole<sample_t, sample_t> outputHighPass;

  sample_t sampleRate = 48000;
  sample_t y1 = 0;
  sample_t rampPhase = 0;
  sample_t rampPhaseIncrement = 80.0 / sampleRate;

  KarplusDrumVoice<sample_t>() { init(); }
  KarplusDrumVoice<sample_t>(const sample_t sr) : sampleRate(sr) { init(); }
  void init() {
    env.set(attackTime, releaseTime, sampleRate);
    ramp.set(0, 1000.0 / 80.0, sampleRate);
    pitchEnv.set(0, 30, sampleRate);
    inputFilter.lowpass(1000, sampleRate);
    snareFilter1.bandpass(750, 1, sampleRate);
    snareFilter2.bandpass(3000, 2, sampleRate);
    exciterEnvelope.set(10, 15, sampleRate);
  }

  inline void setGate(sample_t gate) {
    env.setGate(gate);
    ramp.setGate(gate);
    pitchEnv.setGate(gate);
  }

  void setAllpassHarmonic(const sample_t ratio) { h1 = ratio; }

  void setSampleRate(sample_t sr) { sampleRate = sr; }

  void setFrequency(const sample_t freq) {
    frequency.set(freq, 5, sampleRate);
    ramp.set(0, 1000.0 / freq, sampleRate);
    rampPhaseIncrement = freq / sampleRate;
  }

  const inline sample_t next() {

    auto soundSourceMappedToHalfCircle =
        SineTable<sample_t, 1024>::lookup(soundSource / 4.0);
    h1 = 1.73;
    pitchEnv.set(0, lerp<sample_t>(30, 300, soundSource), sampleRate);
    sample_t pitchEnvelopeSample = pitchEnv.next();
    sample_t bendDepth = lerp<sample_t>(1, 100, soundSource);
    const sample_t freq =
        frequency.next() + bendDepth * pitchBendDepth * pitchEnvelopeSample;
    const sample_t dtime = (sampleRate / freq);
    const sample_t allpassDelayTimeSamples = (sampleRate / (h1 * freq));

    sample_t noise = exciterNoise.next();

    sample_t snareEnvSample = exciterEnvelope.next();
    sample_t exciter = inputFilter.next(noise * rampPhase) +
                       SineTable<sample_t, 1024>::lookup(1.0 - rampPhase);

    exciter = clip<sample_t>(exciter * (1 + 100 * soundSource));
    exciterHighPass.highpass(lerp<sample_t>(10, 10000, soundSource),
                             sampleRate);
    exciter =
        lerp<sample_t>(exciter, exciterHighPass.next(exciter), soundSource);

    rampPhase -= rampPhaseIncrement;
    if (rampPhase < 0)
      rampPhase = 0;

    exciter *= 5.0;

    sample_t s1 = exciter;
    delay.delayTimeSamples = dtime;

    sample_t s2 = delay.next(y1);
    s2 *= feedback;
    apf.g = allpassFilterGain;
    apf.delayTimeSamples = allpassDelayTimeSamples;
    s2 = apf.next(s2);
    lp.lowpass(freq * (1 + 10 * soundSource), sampleRate);
    hp.lowpass(freq * (1 + 10 * soundSource), sampleRate);
    // lp.b1 = b1Coefficient;
    s2 = lerp<sample_t>(lp.next(s2), hp.next(s2), soundSource);

    y1 = s1 + s2;

    snareFollower.setReleaseTime(
        lerp<sample_t>(0.001, 0.2, soundSourceMappedToHalfCircle), sampleRate);
    sample_t snareEnvelopeSample = snareFollower.next(y1);

    snareFilter1.bandpass(lerp<sample_t>(500, 2000, soundSource), 1.7,
                          sampleRate);
    snareFilter2.bandpass(
        lerp<sample_t>(2200, 11000, soundSource * soundSource), 2.3,
        sampleRate);
    auto snares = snareEnvelopeSample * noise * soundSource;
    snares = snareFilter1.next(snares) * 2 + snareFilter2.next(snares);

    env.set(attackTime, releaseTime, sampleRate);
    sample_t envelopeSample = env.next();
    if (env.stage == ADEnvelope<sample_t>::Stage::OFF) {
      active = false;
    }

    auto drum = (snares + y1);
    outputHighPass.highpass(lerp<sample_t>(5, 11000, soundSource), sampleRate);
    drum = lerp<sample_t>(drum, outputHighPass.next(drum), soundSource);

    return clip(drum * (envelopeSample + pitchEnvelopeSample * 2)) * gain;
  }
};

// template <typename sample_t>
// struct KarplusDrumSynthesizer
//     : AbstractPolyphonicSynthesizer<sample_t, KarplusDrumVoice<sample_t>> {
//
//   inline void setFilterCutoff(sample_t value) {
//     value = clamp<sample_t>(value, 0, 1);
//     value = lerp<sample_t>(0.125, 2, value);
//     for (auto &voice : this->voices) {
//       voice.h1 = value;
//     }
//   }
//   inline void setFilterQuality(sample_t value) {
//     value = clamp<sample_t>(value, 0, 1);
//     value = lerp<sample_t>(-0.999, 0.999, value);
//     for (auto &voice : this->voices) {
//       voice.allpassFilterGain = value;
//     }
//   }
//   inline void setSoundSource(sample_t value) {
//     for (auto &voice : this->voices) {
//       voice.soundSource = value;
//     }
//   }
//   inline void setAttackTime(sample_t value) {
//     value *= 1000;
//     for (auto &voice : this->voices) {
//       voice.attackTime = value;
//     }
//   }
//   inline void setReleaseTime(sample_t value) {
//     value *= 1000;
//     for (auto &voice : this->voices) {
//       voice.releaseTime = value;
//     }
//   }
// };
template <typename sample_t> struct KarplusStringVoice {
  sample_t active = false;

  static const size_t MAX_DELAY_SAMPS =
      static_cast<size_t>((70.0 * 48000.0) / 1000.0);

  sample_t exciterMix = 0.5;
  sample_t allpassFilterGain = -0.5;
  sample_t frequency = 440;
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
  void setGate(sample_t gate) {
    env.setGate(gate);
    exciterEnvelope.setGate(gate);
  }

  const inline sample_t next() {

    const sample_t dtime = (sampleRate / frequency);
    const sample_t allpassDelayTimeSamples = (sampleRate / (h1 * frequency));

    exciterTone.setFrequency(frequency, sampleRate);
    sample_t tone = exciterTone.next();
    sample_t noise = exciterNoise.next();

    sample_t exciterLevel = exciterEnvelope.next();
    sample_t exciter = linearXFade4<sample_t>(
        noise, exciterLevel > 0.99, exciterLevel * 2.0 - 1.0,
        algae::dsp::oscillator::SineTable<sample_t, 256>::lookup(exciterLevel),
        soundSource);
    exciter = exciter *= exciterLevel;
    exciter *= 4;

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

template <typename sample_t>
struct KarplusStrongSynthesizer
    : AbstractMonophonicSynthesizer<sample_t,
                                    KarplusStrongSynthesizer<sample_t>> {
  sample_t gain = 1;
  KarplusStringVoice<sample_t> voice;
  sample_t sampleRate = 48000;

  KarplusStrongSynthesizer<sample_t>() { setSampleRate(sampleRate); }
  inline void setSampleRate(sample_t sr) {
    sampleRate = sr;
    voice.setSampleRate(sampleRate);
  }
  inline const sample_t next() {
    sample_t out = 0;
    return (voice.next() * gain);
  }

  inline void process(sample_t *buffer, const size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; ++i) {
      buffer[i] = next();
    }
  }

  inline void setGain(sample_t value) { gain = value; }
  inline void setFilterCutoff(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    value = lerp<sample_t>(0.125, 2, value);
    voice.h1 = value;
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    value = lerp<sample_t>(-0.999, 0.999, value);

    voice.allpassFilterGain = value;
  }
};
