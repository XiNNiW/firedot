#pragma once
#include "SDL_log.h"
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
    return linearXFade4(triangleSample * 20, squareSample, sawSample,
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
  Parameter<sample_t> frequency;
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
  sample_t attackTime = 1;
  sample_t releaseTime = 1000;
  sample_t soundSource = 0;
  sample_t filterCutoff = 0;
  sample_t filterQuality = 0;
  sample_t detune = 15;
  sample_t pitchModulationDepth = 1000;
  sample_t phi = 0;
  sample_t gain = 0;
  sample_t active = 0;
  sample_t sampleRate = 48000;

  SubtractiveDrumSynthVoice() { init(); }

  inline void init() {
    env.set(1, 500, sampleRate);
    env.clapDensity = 3;
    timbreEnv.set(0, 10, sampleRate);
    pitchEnv.set(0, 25, sampleRate);
  }

  inline void setSampleRate(sample_t _sampleRate) { sampleRate = _sampleRate; }
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
    lp2.lowpass(15000 * filterCutoff, 0.01, sampleRate);
    auto bp1Freq = lerp<sample_t>(100, 10000, soundSource);
    auto bp2Freq = bp1Freq - detune;
    bp1Freq += detune;
    bp1.bandpass(bp1Freq, filterQuality, sampleRate);
    bp2.bandpass(bp2Freq, filterQuality, sampleRate);
    hp1.highpass(lerp<sample_t>(15, 80, soundSource), 0.1, sampleRate);
    auto out = bp1.next(oscillatorSample) + bp2.next(oscillatorSample);
    out *= soundSource;
    out += lp1.next(oscillatorSample) * (1 - soundSource);
    out = lp2.next(out);
    out = hp1.next(out);

    // auto out = oscillatorSample;

    env.set(attackTime + soundSource * 30, releaseTime, sampleRate);
    auto envelopeSample = env.next();
    envelopeSample *= envelopeSample;
    envelopeSample *= envelopeSample;
    envelopeSample *= envelopeSample;
    // envelopeSample *= envelopeSample;

    if (env.stage == ClapEnvelope<sample_t>::OFF) {
      active = false;
    }
    // SDL_Log("val: %f", f);

    return clip(out * (envelopeSample + (pitchEnvSample * 2))) * gain;
  }
};

template <typename sample_t> struct SubtractiveDrumSynth {
  static const size_t MAX_VOICES = 8;
  SubtractiveDrumSynthVoice<sample_t> voices[MAX_VOICES];
  int notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
  sample_t gain = 1;
  int voiceIndex = 0;
  sample_t sampleRate = 48000;

  inline void setSampleRate(sample_t sr) {
    sampleRate = sr;
    for (auto &voice : voices) {
      voice.setSampleRate(sampleRate);
    }
  }

  SubtractiveDrumSynth<sample_t>() {
    for (auto &voice : voices) {
      voice.setSampleRate(sampleRate);
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
      SDL_Log("drum note on (%f, %f) for %d", note, velocity, voiceIndex);
      voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
      voices[voiceIndex].gain = velocity / 127.0;
      voices[voiceIndex].env.setGate(true);
      voices[voiceIndex].pitchEnv.setGate(true);
      voices[voiceIndex].timbreEnv.setGate(true);
      voices[voiceIndex].active = true;
      notes[voiceIndex] = note;
      voiceIndex = (voiceIndex + 1) % MAX_VOICES;
    } else {
      for (size_t i = 0; i < MAX_VOICES; i++) {
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
      voice.soundSource = value;
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

template <typename sample_t> struct SubtractiveVoice {
  sample_t sampleRate = 48000.0;
  sample_t active = false;
  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  sample_t gain = 1;
  sample_t filterCutoff = 19000;
  sample_t filterQuality = 0.1;
  sample_t attackTime = 10;
  sample_t releaseTime = 1000;

  MultiOscillator<sample_t> osc;
  Biquad<sample_t, sample_t> filter;
  ASREnvelope<sample_t> env;

  SubtractiveVoice<sample_t>() { init(); }

  inline void init() {
    filter.lowpass(filterCutoff, filterQuality, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);
  }

  inline void setSampleRate(sample_t sampleRate) {
    sampleRate = sampleRate;
    init();
  }

  inline const sample_t next() {
    auto nextFrequency = frequency.next();
    osc.setFrequency(nextFrequency, sampleRate);
    auto out = osc.next();
    env.set(attackTime, releaseTime, sampleRate);
    auto envelopeSample = env.next();
    if (env.stage == ASREnvelope<sample_t>::Stage::OFF) {
      active = false;
    }
    filter.lowpass(filterCutoff, filterQuality, sampleRate);

    out = filter.next(out);
    return out * envelopeSample;
  }
};

template <typename sample_t> struct SubtractiveSynthesizer {
  static const size_t MAX_VOICES = 8;
  size_t voiceIndex = 0;
  SubtractiveVoice<sample_t> voices[MAX_VOICES];
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
      SDL_Log("subtractive note on (%f, %f) for %d", note, velocity,
              voiceIndex);
      voices[voiceIndex].env.setGate(true);
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
          voices[i].env.setGate(false);
          notes[i] = -1;
          break;
        }
      }
    }
  }
  inline void bendNote(const sample_t note, const sample_t destinationNote) {
    SDL_Log("pitch bend for %d to %d", note, destinationNote);
    for (size_t i = 0; i < MAX_VOICES; ++i) {
      if (notes[i] == note) {
        SDL_Log("found note!");
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
    value = clamp<sample_t>(value, 0, 1);
    for (auto &voice : voices) {
      voice.osc.oscMix = value;
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
