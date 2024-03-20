#pragma once

#include "arena.h"
#include "synthesis_abstract.h"
#include "synthesis_clap_envelope.h"
#include "synthesis_mixing.h"
#include "synthesis_parameter.h"
#include <algae.h>
#include <cmath>
#include <math.h>

using algae::dsp::_Filter;
using algae::dsp::control::ADEnvelope;
using algae::dsp::control::ASREnvelope;
// using algae::dsp::filter::Allpass2Comb;
using algae::dsp::filter::Biquad;
// using algae::dsp::filter::InterpolatedDelay;
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

template <typename sample_t> struct PickDirectionFilter {
  sample_t p = 0.5;
  sample_t y1 = 0;
  inline const sample_t next(sample_t x) {
    y1 = x * (1 - p) + y1 * p;
    return y1;
  }
};

template <typename sample_t> struct PickPositionCombFilter {
  sample_t delayTime = 4800;
  size_t maxDelaySamps;
  int indexMask;
  Arena *arena = NULL;
  sample_t *delayLine = NULL;
  int writeIndex = 0;
  PickPositionCombFilter(Arena *_arena, size_t _maxDelaySamps)
      : arena(_arena),
        maxDelaySamps(algae::dsp::math::nextPowerOf2(_maxDelaySamps)),
        indexMask(maxDelaySamps - 1) {}
  inline const bool allocateDelayLine() {
    if ((delayLine == NULL) &&
        (arena->canAllocArray<sample_t>(maxDelaySamps))) {
      delayLine = arena->pushArray<sample_t>(maxDelaySamps);
      algae::dsp::block::empty(maxDelaySamps, delayLine);
      return true;
    }
    return false;
  }
  inline const void set(sample_t beta, sample_t totalLoopDelaySamples) {
    delayTime = beta * totalLoopDelaySamples;
  }
  inline const sample_t next(sample_t x) {
    int readIndex = writeIndex - delayTime;
    readIndex &= indexMask;
    const sample_t yn = x + delayLine[readIndex];
    delayLine[writeIndex] = x;
    writeIndex++;
    writeIndex &= indexMask;
    return yn;
  }
};

template <typename sample_t> struct StringDampingFilter {
  sample_t x1 = 0;
  sample_t c1 = 1;
  sample_t c2 = 0;
  inline const void set(sample_t freq, sample_t decayTimeSeconds,
                        sample_t stretchingFactor) {
    sample_t gain = pow(0.001, 1 / (freq * decayTimeSeconds));
    c1 = (1 - stretchingFactor) * gain;
    c2 = stretchingFactor;
  }
  inline const sample_t next(sample_t x) {
    auto yn = c1 * x + c2 * x1;
    x1 = x;
    return yn;
  }
};

template <typename sample_t> struct StringDampingFilter2 {
  sample_t x1 = 0;
  sample_t x2 = 0;
  sample_t rho = 1;
  sample_t h0 = 1;
  sample_t h1 = 0;
  inline const void set(sample_t freq, sample_t decayTimeSeconds,
                        sample_t brightness) {
    rho = pow(0.001, 1 / (freq * decayTimeSeconds));
    h0 = (1 + brightness) / 2.0;
    h1 = (1 - brightness) / 4.0;
  }
  inline const sample_t next(sample_t x) {
    auto yn = rho * (h0 * x1 + h1 * (x + x2));
    x2 = x1;
    x1 = x;
    return yn;
  }
};

template <typename sample_t> struct DynamicLevelLowpassFilter {
  sample_t x1 = 0;
  sample_t y1;
  sample_t l = 1;
  sample_t l0 = 1;
  sample_t lw = 0;
  sample_t lgain = 0;
  sample_t lpole2 = 0;
  inline void set(sample_t levelDB, sample_t freq, sample_t sampleRate) {
    l = levelDB;
    l0 = pow(l, 1.0 / 3.0);
    lw = algae::dsp::math::Pi<sample_t>() * freq / sampleRate;
    lgain = lw / (1 + lw);
    lpole2 = (1 - lw) / (1 + lw);
  }
  inline const sample_t next(sample_t x) {
    sample_t yn = (l * l0 * x) + ((1 - l) * (x * lgain + y1 * lpole2));
    y1 = yn;
    x1 = x;
    return yn;
  }
};

template <typename sample_t> struct Allpass2CombArena {
  size_t maxDelaySamps;
  int indexMask;
  Arena *arena = NULL;
  sample_t *yn = NULL;
  sample_t *xn = NULL;
  int writeIndex = 0;
  sample_t delayTimeSamples = 1;
  sample_t g = 0.5;
  Allpass2CombArena(Arena *_arena, size_t _maxDelaySamps)
      : arena(_arena),
        maxDelaySamps(algae::dsp::math::nextPowerOf2(_maxDelaySamps)),
        indexMask(maxDelaySamps - 1) {}

  inline const bool allocateDelayLine() {
    if ((xn == NULL) && (arena->canAllocArray<sample_t>(maxDelaySamps * 2))) {
      xn = arena->pushArray<sample_t>(maxDelaySamps);
      yn = arena->pushArray<sample_t>(maxDelaySamps);
      algae::dsp::block::empty(maxDelaySamps, xn);
      algae::dsp::block::empty(maxDelaySamps, yn);
      return true;
    }
    return false;
  }

  void set(sample_t delayTimeSamps, sample_t g) {
    this->delayTimeSamples = delayTimeSamps;
    this->g = g;
  }

  void setDelayTimeMillis(sample_t delayTimeMillis, sample_t samplerate) {
    delayTimeSamples = (delayTimeMillis * samplerate) / 1000.0;
  }

  inline const sample_t tap(int pos) {
    return yn[(writeIndex - pos) & indexMask];
  }

  const inline sample_t next(const sample_t in) {
    int readIndex = (this->writeIndex - this->delayTimeSamples);
    readIndex &= indexMask;
    const sample_t _yn =
        this->g * in + this->xn[readIndex] - this->g * this->yn[readIndex];
    this->yn[this->writeIndex] = _yn;
    this->xn[this->writeIndex] = in;
    this->writeIndex++;
    this->writeIndex &= indexMask;
    return _yn;
  }
};

template <typename sample_t,
          const sample_t (*interp)(const sample_t, const sample_t,
                                   const sample_t) =
              algae::dsp::math::lerp<sample_t>>
struct InterpolatedDelayArena {
  size_t bufferSize = 0; // = algae::dsp::math::nextPowerOf2(MAX_DELAY_SAMPS);
  int indexMask = 0;     // = int(BUFFER_SIZE - 1);
  Arena *arena = NULL;
  sample_t *buffer = NULL;
  int writePosition = 0;
  sample_t delayTimeSamples = 0; // = BUFFER_SIZE / 2;

  InterpolatedDelayArena(Arena *_arena, size_t _maxDelaySamps)
      : arena(_arena),
        bufferSize(algae::dsp::math::nextPowerOf2(_maxDelaySamps)),
        delayTimeSamples(bufferSize / 2.0), indexMask(bufferSize - 1) {}

  inline const bool allocateDelayLine() {
    if ((buffer == NULL) && (arena->canAllocArray<sample_t>(bufferSize))) {
      buffer = arena->pushArray<sample_t>(bufferSize);
      algae::dsp::block::empty(bufferSize, buffer);
      return true;
    }
    return false;
  }

  void setDelayTimeMillis(sample_t delayTimeMillis, sample_t samplerate) {
    delayTimeSamples = (delayTimeMillis * samplerate) / 1000.0;
  }

  inline const sample_t tap(int pos) {
    return buffer[(writePosition - pos) & indexMask];
  }

  inline const sample_t next(const sample_t in) {
    int dtWhole = floor(delayTimeSamples);
    sample_t dtMantissa = delayTimeSamples - dtWhole;
    int r1 = (writePosition - dtWhole) & indexMask;
    int r2 = (r1 + 1) & indexMask;
    sample_t out = interp(buffer[r1], buffer[r2], dtMantissa);
    buffer[writePosition] = in;
    writePosition++;
    writePosition &= indexMask;
    return out;
  }
};

template <typename sample_t> struct StringVoice {
  static constexpr size_t MAX_DELAY_SAMPS = (70.0 * 48000.0) / 1000.0;
  Parameter<sample_t> frequency = 440;
  sample_t soundSource = 0;
  Parameter<sample_t> brightness = 1;
  Parameter<sample_t> inharmonicity = 0;
  Parameter<sample_t> gain = 1;
  sample_t attackTime = 1;
  sample_t releaseTime = 1000;
  sample_t sampleRate = 48000;
  sample_t y1 = 0;
  WhiteNoise<sample_t> exciterNoise;
  SinOscillator<sample_t, sample_t> exciterTone;
  ADEnvelope<sample_t> exciterEnvelope;
  PickDirectionFilter<sample_t> pickPositionFilter;
  PickPositionCombFilter<sample_t> pickPositionCombFilter;
  Allpass2CombArena<sample_t> inharmonicFilter;
  InterpolatedDelayArena<sample_t> delay;
  StringDampingFilter2<sample_t> stringDampingFilter;
  DynamicLevelLowpassFilter<sample_t> dynamicLevelLowpassFilter;
  Biquad<sample_t, sample_t> filter;

  StringVoice(Arena *arena)
      : pickPositionCombFilter(
            PickPositionCombFilter<sample_t>(arena, MAX_DELAY_SAMPS)),
        inharmonicFilter(Allpass2CombArena<sample_t>(arena, MAX_DELAY_SAMPS)),
        delay(InterpolatedDelayArena<sample_t>(arena, MAX_DELAY_SAMPS)) {}

  void init() {
    pickPositionCombFilter.allocateDelayLine();
    inharmonicFilter.allocateDelayLine();
    delay.allocateDelayLine();
  }

  void setSampleRate(sample_t sr) { sampleRate = sr; }

  void setGate(sample_t gate) { exciterEnvelope.setGate(gate); }

  const sample_t next() {
    const sample_t f = frequency.next();
    const sample_t delaytime = (sampleRate / f);

    exciterTone.setFrequency(f, sampleRate);
    sample_t tone = exciterTone.next();
    sample_t noise = exciterNoise.next();
    sample_t g = gain.next();

    auto beta = algae::dsp::math::clamp<sample_t>(soundSource, 0.01, 0.99);

    pickPositionCombFilter.set(beta, delaytime);
    pickPositionFilter.p = beta;
    dynamicLevelLowpassFilter.set(g, f, sampleRate);

    exciterEnvelope.set(attackTime, 33, sampleRate);

    sample_t exciter =
        lerp<sample_t>(noise, tone * 0.5, soundSource) * exciterEnvelope.next();
    exciter = pickPositionFilter.next(exciter);
    exciter = pickPositionCombFilter.next(exciter);
    exciter = dynamicLevelLowpassFilter.next(exciter);

    auto nextBrightness = brightness.next();
    stringDampingFilter.set(f, releaseTime / 1000.0, nextBrightness);
    inharmonicFilter.set(delaytime * 0.5 * (1 + (1 - inharmonicity.next())),
                         0.5);

    sample_t output = (exciter + y1);
    delay.delayTimeSamples = delaytime;
    output = delay.next(output);
    y1 = tanh(y1) * 0.9999;
    y1 = stringDampingFilter.next(output);
    y1 = inharmonicFilter.next(y1);

    filter.lowpass(
        lerp<sample_t>(f, algae::dsp::math::clamp<sample_t>(16 * f, 500, 19000),
                       nextBrightness),
        0.1, sampleRate);
    output = filter.next(output);

    return tanh(output) * g;
  }
};
template <typename sample_t> struct KarplusStrongSynthesizer {
  Parameter<sample_t> gain = Parameter<sample_t>(1);
  StringVoice<sample_t> voice;
  sample_t sampleRate = 48000;
  Arena *arena = NULL;

  KarplusStrongSynthesizer<sample_t>(Arena *_arena)
      : arena(_arena), voice(StringVoice<sample_t>(_arena)) {
    init();
  }

  inline void init() { voice.init(); }

  inline void clear() { arena->clear(); }

  inline const sample_t next() { return voice.next() * 0.5; }

  inline void process(sample_t *buffer, const size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; ++i) {
      buffer[i] = next();
    }
  }

  inline void setSampleRate(sample_t sr) {
    sampleRate = sr;
    voice.setSampleRate(sampleRate);
  }
  inline void setGate(sample_t gate) { voice.setGate(gate); }
  inline void setGain(sample_t value) { voice.gain.set(value, 5, sampleRate); }
  inline void setFrequency(sample_t value) {
    voice.frequency.set(value, 33, sampleRate);
  }
  inline void setSoundSource(sample_t value) { voice.soundSource = value; }

  inline void setFilterCutoff(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    voice.brightness.set(value, 5, sampleRate);
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    voice.inharmonicity.set(value, 5, sampleRate);
  }
  inline void setAttackTime(sample_t value) {
    value = clamp<sample_t>(value, 0, 1) * 1000.0;
    voice.attackTime = value;
  }
  inline void setReleaseTime(sample_t value) {
    value = clamp<sample_t>(value, 0.001, 1) * 1000.0;
    voice.releaseTime = value;
  }
  inline void bendNote(const sample_t frequency,
                       const sample_t destinationFrequency) {
    voice.frequency = destinationFrequency;
  }
};
// template <typename sample_t> struct KarplusStringVoice {
//   sample_t active = false;
//
//   static const size_t MAX_DELAY_SAMPS =
//       static_cast<size_t>((70.0 * 48000.0) / 1000.0);
//
//   sample_t exciterMix = 0.5;
//   Parameter<sample_t> allpassFilterGain = Parameter<sample_t>(-0.5);
//   Parameter<sample_t> frequency = Parameter<sample_t>(440);
//   Parameter<sample_t> feedback = Parameter<sample_t>(0.99);
//   Parameter<sample_t> b1Coefficient = Parameter<sample_t>(1.0);
//   Parameter<sample_t> h1 = Parameter<sample_t>(1.0);
//   sample_t attackTime = 1;
//   sample_t releaseTime = 1000;
//
//   ASREnvelope<sample_t> env;
//   WhiteNoise<sample_t> exciterNoise;
//   SinOscillator<sample_t, sample_t> exciterTone;
//   ADEnvelope<sample_t> exciterEnvelope;
//   Allpass2Comb<sample_t, MAX_DELAY_SAMPS> apf;
//   InterpolatedDelay<sample_t, MAX_DELAY_SAMPS> delay;
//   Onepole<sample_t, sample_t> inputFilter;
//   Onezero<sample_t> lp;
//   sample_t soundSource = 0;
//
//   sample_t sampleRate = 48000;
//   sample_t y1 = 0;
//
//   KarplusStringVoice<sample_t>() { init(); }
//   KarplusStringVoice<sample_t>(const sample_t sr) : sampleRate(sr) { init();
//   }
//
//   void init() {
//     env.set(attackTime, releaseTime, sampleRate);
//     inputFilter.lowpass(19000, sampleRate);
//     exciterEnvelope.set(0.1, 1, sampleRate);
//   }
//
//   void setAllpassHarmonic(const sample_t ratio) { h1 = ratio; }
//
//   void setSampleRate(sample_t sr) { sampleRate = sr; }
//
//   void setGate(sample_t gate) {
//     env.setGate(gate);
//     exciterEnvelope.setGate(gate);
//   }
//
//   const inline sample_t next() {
//     auto nextFrequency = frequency.next();
//     const sample_t dtime = (sampleRate / nextFrequency);
//     const sample_t allpassDelayTimeSamples =
//         (sampleRate / (h1.next() * nextFrequency));
//
//     exciterTone.setFrequency(nextFrequency, sampleRate);
//     sample_t tone = exciterTone.next();
//     sample_t noise = exciterNoise.next();
//
//     sample_t exciterLevel = exciterEnvelope.next();
//     sample_t exciter = linearXFade4<sample_t>(
//         noise, exciterLevel > 0.99, exciterLevel * 2.0 - 1.0,
//         algae::dsp::oscillator::SineTable<sample_t,
//         256>::lookup(exciterLevel), soundSource);
//     exciter = inputFilter.next(exciter);
//     exciter *= exciterLevel;
//     exciter *= 4;
//
//     sample_t s1 = exciter;
//     // sample_t s1 = inputFilter.next(exciter);
//     delay.delayTimeSamples = dtime;
//
//     sample_t s2 = delay.next(y1);
//     s2 *= feedback.next();
//     apf.g = allpassFilterGain.next();
//     apf.delayTimeSamples = allpassDelayTimeSamples;
//     s2 = apf.next(s2);
//     lp.b1 = b1Coefficient.next();
//     s2 = lp.next(s2);
//
//     y1 = s1 + s2;
//
//     env.set(attackTime, releaseTime, sampleRate);
//     sample_t envelopeSample = env.next();
//     //   return algae::dsp::math::tanh_approx_pade(y1 * envelopeSample * 100)
//     //   *
//     //          (1.0 / 3.0);
//     return tanh(y1 * envelopeSample);
//   }
// };
//
// template <typename sample_t>
// struct KarplusStrongSynthesizer
//     : AbstractMonophonicSynthesizer<sample_t,
//                                     KarplusStrongSynthesizer<sample_t>> {
//   Parameter<sample_t> gain = Parameter<sample_t>(1);
//   KarplusStringVoice<sample_t> voice;
//   sample_t sampleRate = 48000;
//
//   KarplusStrongSynthesizer<sample_t>() { setSampleRate(sampleRate); }
//   inline void setSampleRate(sample_t sr) {
//     sampleRate = sr;
//     voice.setSampleRate(sampleRate);
//   }
//
//   inline void setSoundSource(sample_t value) { voice.soundSource = value; }
//
//   inline void setFilterCutoff(sample_t value) {
//     value = clamp<sample_t>(value, 0, 1);
//     value = lerp<sample_t>(0.125, 2, value);
//     voice.h1.set(value, 33, sampleRate);
//   }
//   inline void setFilterQuality(sample_t value) {
//     value = clamp<sample_t>(value, 0, 1);
//     value = lerp<sample_t>(-0.999, 0.999, value);
//
//     voice.allpassFilterGain.set(value, 33, sampleRate);
//   }
// };
