#pragma once
#include "SDL_log.h"
#include "synthesis_abstract.h"
#include "synthesis_clap_envelope.h"
#include "synthesis_parameter.h"
#include <algae.h>
#include <cmath>
#include <cstddef>

using algae::dsp::control::ADEnvelope;
using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Biquad;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::oscillator::blep;
using algae::dsp::oscillator::computePhaseIncrement;
using algae::dsp::oscillator::SineTable;
using algae::dsp::oscillator::WhiteNoise;

template <typename sample_t> struct MultiOscillator {
  WhiteNoise<sample_t> noise;
  sample_t oscMix = 0;
  sample_t phase_increment = 0;
  sample_t phase = 0;
  sample_t y1 = 0;

  inline void setFrequency(const sample_t frequency,
                           const sample_t sampleRate) {
    phase_increment = computePhaseIncrement<sample_t>(frequency, sampleRate);
  }

  inline const sample_t next() {

    auto phi = phase_increment;

    // compute SAW
    sample_t sawSample = (2.0 * phase) - 1.0;
    sample_t endOfPhaseStep = blep<sample_t>(phase, phi);
    sawSample -= endOfPhaseStep;

    // compute SQUARE
    const sample_t pwidth = 0.5;
    sample_t squareSample = phase < pwidth ? 1 : -1;
    squareSample += endOfPhaseStep;
    sample_t dutyCycleStep =
        blep<sample_t>(fmod(phase + (1.0 - pwidth), 1.0), phi);
    squareSample -= dutyCycleStep;

    // compute TRIANGLE
    sample_t triangleSample = phi * squareSample + (1.0 - phi) * y1;
    y1 = triangleSample;

    // compute noise
    sample_t noiseSample = noise.next();

    // update phase
    phase += phase_increment; // + pm
    phase = phase > 1 ? phase - 1 : phase;

    // mix output
    return linearXFade4(triangleSample * 3, squareSample, sawSample,
                        noiseSample, oscMix);
  }
};

// template <typename sample_t> struct vMultiOscillator {
//   const int MAX_NUMBER_OF_VOICES;
//   WhiteNoise<sample_t> *noise;
//   sample_t *oscMix = 0;
//   sample_t *phase_increment = 0;
//   sample_t *phase = 0;
//   sample_t *y1 = 0;
//
//   vMultiOscillator<sample_t>(const int _maxNumberOfVoices)
//       : MAX_NUMBER_OF_VOICES(_maxNumberOfVoices) {
//     noise = new WhiteNoise<sample_t>[MAX_NUMBER_OF_VOICES]();
//     oscMix = new sample_t[MAX_NUMBER_OF_VOICES]();
//     phase_increment = new sample_t[MAX_NUMBER_OF_VOICES]();
//     phase = new sample_t[MAX_NUMBER_OF_VOICES]();
//     y1 = new sample_t[MAX_NUMBER_OF_VOICES]();
//   }
//   ~vMultiOscillator<sample_t>() {
//     delete[] noise;
//     delete[] oscMix;
//     delete[] phase_increment;
//     delete[] phase;
//     delete[] y1;
//   }
//
//   inline void setFrequency(const int index, const sample_t frequency,
//                            const sample_t sampleRate) {
//     phase_increment[index] =
//         computePhaseIncrement<sample_t>(frequency, sampleRate);
//   }
//
//   inline const sample_t next() {
//     sample_t voiceOuts[MAX_NUMBER_OF_VOICES];
//     for (int i = 0; i < MAX_NUMBER_OF_VOICES; ++i) {
//       auto phi = phase_increment[i];
//
//       // compute SAW
//       sample_t sawSample = (2.0 * phase[i]) - 1.0;
//       sample_t endOfPhaseStep = blep<sample_t>(phase[i], phi);
//       sawSample -= endOfPhaseStep;
//
//       // compute SQUARE
//       const sample_t pwidth = 0.5;
//       sample_t squareSample = phase[i] < pwidth ? 1 : -1;
//       squareSample += endOfPhaseStep;
//       sample_t dutyCycleStep =
//           blep<sample_t>(fmod(phase[i] + (1.0 - pwidth), 1.0), phi);
//       squareSample -= dutyCycleStep;
//
//       // compute TRIANGLE
//       sample_t triangleSample = phi * squareSample + (1.0 - phi) * y1[i];
//       y1[i] = triangleSample;
//
//       // compute noise
//       sample_t noiseSample = noise[i].next();
//
//       // update phase
//       phase[i] += phase_increment[i]; // + pm
//       phase[i] = phase[i] > 1 ? phase[i] - 1 : phase[i];
//
//       // mix output
//       voiceOuts[i] = linearXFade4(triangleSample * 2, squareSample,
//       sawSample,
//                                   noiseSample, oscMix[i]);
//     }
//
//     sample_t out = 0;
//     for (auto sample : voiceOuts) {
//       out += sample;
//     }
//
//     return out;
//   }
// };

template <typename sample_t> struct SubtractiveDrumSynthVoice {
  ClapEnvelope<sample_t> env;
  ADEnvelope<sample_t> timbreEnv;
  ADEnvelope<sample_t> pitchEnv;
  Biquad<sample_t, sample_t> lp1;
  Biquad<sample_t, sample_t> lp2;
  Biquad<sample_t, sample_t> bp1;
  Biquad<sample_t, sample_t> bp2;
  Biquad<sample_t, sample_t> hp1;
  MultiOscillator<sample_t> osc1;
  MultiOscillator<sample_t> osc2;
  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  Parameter<sample_t> filterCutoff = Parameter<sample_t>(19000);
  Parameter<sample_t> filterQuality = Parameter<sample_t>(0);
  sample_t soundSource = 0;
  sample_t attackTime = 1;
  sample_t releaseTime = 1000;
  sample_t detune = 15;
  sample_t pitchModulationDepth = 1000;
  sample_t active = false;
  sample_t sampleRate = 48000;
  sample_t phi = 0;

  SubtractiveDrumSynthVoice() { init(); }

  inline void init() {
    env.set(1, 500, sampleRate);
    env.clapDensity = 3;
    timbreEnv.set(0, 10, sampleRate);
    pitchEnv.set(0, 25, sampleRate);
  }

  inline void setSampleRate(sample_t sampleRate) {
    this->sampleRate = sampleRate;
  }

  inline void setGate(sample_t gate) {
    env.setGate(gate);
    pitchEnv.setGate(gate);
    timbreEnv.setGate(gate);
  }

  inline const sample_t next() {
    auto soundSourceMappedToHalfCircle =
        SineTable<sample_t, 1024>::lookup(soundSource / 4.0);
    timbreEnv.set(lerp<sample_t>(10, 50, soundSource),
                  lerp<sample_t>(10, 75, soundSource), sampleRate);
    pitchEnv.set(0, lerp<sample_t>(15, 35, soundSource), sampleRate);

    auto f = frequency.next();
    auto pitchEnvSample = pitchEnv.next();
    f += pitchEnvSample * pitchModulationDepth;
    auto f1 = fmax(f - detune * soundSource, 0);
    auto f2 = f + detune * soundSource;

    osc1.setFrequency(f1, sampleRate);
    osc2.setFrequency(f2, sampleRate);
    auto timbre = clip(timbreEnv.next() + soundSource);
    osc1.oscMix = timbre;
    osc2.oscMix = timbre;

    auto oscillatorSample = osc1.next() + osc2.next();

    lp1.lowpass(pitchEnvSample * pitchModulationDepth +
                    lerp<sample_t>(65, 10000, soundSource),
                1 - pitchEnvSample, sampleRate);
    auto nextFilterCutoff = filterCutoff.next();
    auto nextFilterQuality = filterQuality.next();
    lp2.lowpass(nextFilterCutoff, 0.01, sampleRate);
    auto bp1Freq = lerp<sample_t>(100, 10000, soundSource);
    auto bp2Freq = bp1Freq - detune;
    bp1Freq += detune;
    bp1.bandpass(bp1Freq, nextFilterQuality, sampleRate);
    bp2.bandpass(bp2Freq, nextFilterQuality, sampleRate);
    hp1.highpass(lerp<sample_t>(15, 80, soundSource), 0.1, sampleRate);
    auto out = bp1.next(oscillatorSample) + bp2.next(oscillatorSample);
    out *= soundSource;
    out += lp1.next(oscillatorSample) * (1 - soundSource);
    out = lp2.next(out);
    out = hp1.next(out);
    out *= 4;

    env.set(attackTime + soundSource * 30, releaseTime, sampleRate);
    auto envelopeSample = env.next();
    envelopeSample *= envelopeSample;
    envelopeSample *= envelopeSample;
    envelopeSample *= envelopeSample;

    if (env.stage == ClapEnvelope<sample_t>::OFF) {
      active = false;
    }

    return clip(out * (envelopeSample + (pitchEnvSample * 4))) * 0.5;
  }
};

template <typename sample_t>
struct SubtractiveDrumSynth
    : AbstractMonophonicSynthesizer<sample_t, SubtractiveDrumSynth<sample_t>> {

  SubtractiveDrumSynthVoice<sample_t> voice;

  inline void setSoundSource(sample_t value) { voice.soundSource = value; }
};

template <typename sample_t> struct SubtractiveVoice {
  sample_t sampleRate = 48000.0;
  Parameter<sample_t> active = Parameter<sample_t>(false);
  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  Parameter<sample_t> soundSource = Parameter<sample_t>(0);
  Parameter<sample_t> filterCutoff = Parameter<sample_t>(19000);
  Parameter<sample_t> filterQuality = Parameter<sample_t>(0.1);
  sample_t attackTime = 10;
  sample_t releaseTime = 1000;

  MultiOscillator<sample_t> osc;
  Biquad<sample_t, sample_t> filter;
  ASREnvelope<sample_t> env;

  SubtractiveVoice<sample_t>() { init(); }

  inline void init() {
    filter.lowpass(filterCutoff.value, filterQuality.value, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);
  }

  inline void setSampleRate(sample_t sampleRate) {
    sampleRate = sampleRate;
    init();
  }

  inline void setGate(sample_t gate) { env.setGate(gate); }

  inline const sample_t next() {
    osc.setFrequency(frequency.next(), sampleRate);
    osc.oscMix = soundSource.next();
    auto out = osc.next();
    env.set(attackTime, releaseTime, sampleRate);
    auto envelopeSample = env.next();
    if (env.stage == ASREnvelope<sample_t>::Stage::OFF) {
      active = false;
    }
    filter.lowpass(filterCutoff.next(), filterQuality.next(), sampleRate);

    out = filter.next(out);
    return out * envelopeSample;
  }
};

template <typename sample_t>
struct SubtractiveSynthesizer
    : AbstractMonophonicSynthesizer<sample_t,
                                    SubtractiveSynthesizer<sample_t>> {
  SubtractiveVoice<sample_t> voice;
};
