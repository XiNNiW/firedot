#pragma once

#include <algae.h>
#include <atomic>
#include <cstddef>
#include <rigtorp/SPSCQueue.h>

using algae::dsp::_Filter;
using algae::dsp::_Generator;
using algae::dsp::control::ADEnvelope;
using algae::dsp::filter::Allpass2Comb;
using algae::dsp::filter::Biquad;
using algae::dsp::filter::InterpolatedDelay;
using algae::dsp::filter::Onepole;
using algae::dsp::filter::ResonantBandpass2ndOrderIIR;
using algae::dsp::filter::SmoothParameter;
using algae::dsp::math::lerp;
using algae::dsp::math::mtof;
using algae::dsp::oscillator::blep;
using algae::dsp::oscillator::PolyBLEPSaw;
using algae::dsp::oscillator::PolyBLEPSquare;
using algae::dsp::oscillator::PolyBLEPTri;
using algae::dsp::oscillator::SinOscillator;
using algae::dsp::oscillator::WhiteNoise;
template <typename sample_t> struct Parameter {
  sample_t value = 0;
  SmoothParameter<sample_t> smoothingFilter;

  inline const sample_t next() { return smoothingFilter.next(value); }
  void set(sample_t newValue, sample_t smoothingTimeMillis,
           sample_t sampleRate) {
    smoothingFilter.set(smoothingTimeMillis, sampleRate);
    value = newValue;
  }
};

template <typename sample_t>
struct Onezero : _Filter<sample_t, Onezero<sample_t>> {
  sample_t x1 = 0;
  sample_t b1 = 0.1;
  const inline sample_t next(const sample_t in) {
    sample_t out = in + b1 * x1;
    x1 = in;
    return out * 0.5;
  }
};

template <typename sample_t>
struct ASREnvelope : _Generator<sample_t, ASREnvelope<sample_t>> {

  enum Stage { OFF, ATTACK, SUSTAIN, RELEASE } stage;
  sample_t phase = 0;
  sample_t attack_increment = 1.0 / 5.0;
  sample_t decay_increment = 1.0 / 4100.0;
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
  };

  inline void setGate(const bool gate) {
    switch (stage) {
    case Stage::OFF: {
      if (gate) {
        phase = 0;
        stage = Stage::ATTACK;
      }
      break;
    }
    case Stage::ATTACK: {
      if (!gate) {
        phase = 0;
        stage = Stage::RELEASE;
      }
      break;
    }
    case Stage::SUSTAIN: {
      if (!gate) {
        phase = 0;
        stage = Stage::RELEASE;
      }
      break;
    }
    case Stage::RELEASE: {
      break;
    }
    }
  }

  inline const sample_t next() {
    sample_t out = 0;
    switch (stage) {
    case Stage::ATTACK: {
      out = phase;
      phase += attack_increment;
      if (phase > 1) {
        phase = 0;
        stage = Stage::SUSTAIN;
      }
      break;
    }
    case Stage::SUSTAIN: {
      out = 1;
      break;
    }
    case Stage::RELEASE: {
      out = 1 - phase;
      phase += decay_increment;
      if (phase > 1) {
        phase = 0;
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
};

template <typename sample_t> struct KarplusStringVoice {
  bool active = false;
  bool gate = false;

  static const size_t MAX_DELAY_SAMPS =
      static_cast<size_t>((70.0 * 48000.0) / 1000.0);

  Parameter<sample_t> filterCutoff;
  Parameter<sample_t> filterQuality;
  Parameter<sample_t> exciterMix;
  Parameter<sample_t> allpassFilterGain;
  Parameter<sample_t> frequency;
  Parameter<sample_t> panPosition;
  Parameter<sample_t> feedback;
  Parameter<sample_t> b1Coefficient;
  Parameter<sample_t> gain;
  Parameter<sample_t> attackTime;
  Parameter<sample_t> releaseTime;

  //
  ADEnvelope<sample_t> env;
  WhiteNoise<sample_t> exciterNoise;
  SinOscillator<sample_t, sample_t> exciterTone;
  ASREnvelope<sample_t> exciterEnvelope;
  Allpass2Comb<sample_t, MAX_DELAY_SAMPS> apf;
  InterpolatedDelay<sample_t, MAX_DELAY_SAMPS> delay;
  Onepole<sample_t, sample_t> inputFilter;
  Onezero<sample_t> lp;

  sample_t sampleRate = 48000;
  sample_t y1 = 0;
  sample_t h1 = 1.73;

  KarplusStringVoice<sample_t>() { init(); }
  KarplusStringVoice<sample_t>(const sample_t sr) : sampleRate(sr) { init(); }
  void init() {
    env.set(0.1, 500, sampleRate);
    inputFilter.lowpass(19000, sampleRate);
    exciterEnvelope.set(0.1, 1, sampleRate);
    allpassFilterGain.value = -0.5;
    frequency.value = 440;
    panPosition.value = 0.5;
    feedback.value = 0.999;
    b1Coefficient.value = 0.999;
    gain.value = 0.8;
  }

  void setSampleRate(sample_t sr) { sampleRate = sr; }

  const inline sample_t next() {

    const sample_t fb = feedback.next();
    const sample_t b1 = b1Coefficient.next();
    const sample_t apfg = allpassFilterGain.next();
    const sample_t panPos = panPosition.next();
    const sample_t freq = frequency.next();
    const sample_t dtime = (sampleRate / freq);
    const sample_t allpassDelayTimeSamples = (sampleRate / (h1 * freq));
    const sample_t _gain = gain.next();

    sample_t tone = exciterTone.next();
    sample_t noise = exciterNoise.next();

    sample_t exciterLevel = exciterEnvelope.next();
    sample_t exciter = lerp(noise, tone, exciterLevel);
    exciter *= exciterLevel;
    exciter *= _gain;

    exciter = inputFilter.next(exciter + gate);

    env.set(attackTime.next(), releaseTime.next(), sampleRate);
    env.setGate(gate);
    if (env.stage == ADEnvelope<sample_t>::Stage::OFF) {
      active = false;
    }

    sample_t s1 = inputFilter.next(exciter);
    delay.delayTimeSamples = dtime;

    sample_t s2 = delay.next(y1);
    s2 *= fb;
    apf.g = apfg;
    apf.delayTimeSamples = allpassDelayTimeSamples;
    s2 = apf.next(s2);
    lp.b1 = b1;
    s2 = lp.next(s2);

    y1 = s1 + s2;
    sample_t e = env.next();
    if ((e + gate) < 0.0001) {
      active = false;
    }

    return y1 * _gain * e;
  }
};

template <typename sample_t> struct KarplusStrongSynthesizer {
  static const size_t MAX_VOICES = 8;
  Parameter<sample_t> gain;
  std::array<KarplusStringVoice<sample_t>, MAX_VOICES> voices;
  std::array<sample_t, MAX_VOICES> notes;
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
    return out * gain.next();
  }

  inline void note(sample_t note, sample_t velocity) {
    voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
    voices[voiceIndex].gain.set(velocity / 127.0, 1, sampleRate);
    voices[voiceIndex].gate = true;
    voices[voiceIndex].active = true;
    voiceIndex = (voiceIndex + 1) % voices.size();
  }

  inline void setGain(sample_t value) { gain.set(value, 5, sampleRate); }
  inline void setFilterCutoff(sample_t value) {
    for (auto &voice : voices) {
      voice.filterCutoff.set(value, 5, sampleRate);
    }
  }
  inline void setFilterQuality(sample_t value) {
    for (auto &voice : voices) {
      voice.filterQuality.set(value, 5, sampleRate);
    }
  }
  inline void setSoundSource(sample_t value) {
    for (auto &voice : voices) {
      voice.exciterMix.set(value, 5, sampleRate);
    }
  }
  inline void setAttackTime(sample_t value) {
    for (auto &voice : voices) {
      voice.attackTime.set(value, 5, sampleRate);
    }
  }
  inline void setReleaseTime(sample_t value) {
    for (auto &voice : voices) {
      voice.releaseTime.set(value, 5, sampleRate);
    }
  }
};

template <typename sample_t>
inline const sample_t linearXFade4(sample_t one, sample_t two, sample_t three,
                                   sample_t four, sample_t mixAmount) {
  mixAmount = mixAmount * 4.0;
  sample_t twoMix = std::min(std::max(mixAmount - 1.0, 0.0), 1.0);
  sample_t threeMix = std::min(std::max(mixAmount - 2.0, 0.0), 1.0);
  sample_t fourMix = std::min(std::max(mixAmount - 3.0, 0.0), 1.0);
  sample_t channelOneMixLevel = std::max(1.0 - mixAmount, 0.0);
  sample_t channelTwoMixLevel = twoMix - threeMix;
  sample_t channelThreeMixLevel = threeMix - fourMix;
  sample_t channelFourMixLevel = fourMix;
  return (one * channelOneMixLevel) + (two * channelTwoMixLevel) +
         (three * channelThreeMixLevel) + (four * channelFourMixLevel);
};

template <typename sample_t> struct MultiOscillator {
  WhiteNoise<sample_t> noise;
  sample_t frequency;
  sample_t oscMix;
  sample_t phase_increment = 0;
  sample_t phase = 0;
  sample_t y1 = 0;
  sample_t sampleRate = 48000;

  inline const sample_t next() {

    phase_increment =
        algae::dsp::oscillator::computePhaseIncrement(frequency, sampleRate);

    // compute SAW
    sample_t sawSample = (2.0 * phase) - 1.0;
    sample_t endOfPhaseStep = blep<sample_t>(phase, phase_increment);
    sawSample -= endOfPhaseStep;

    // compute SQUARE
    const sample_t pwidth = 0.5;
    sample_t squareSample = phase < pwidth ? 1 : -1;
    squareSample += endOfPhaseStep;
    sample_t dutyCycleStep =
        blep<sample_t>(fmod(phase + (1.0 - pwidth), 1.0), phase_increment);
    squareSample -= dutyCycleStep;

    // compute TRIANGLE
    sample_t triangleSample =
        phase_increment * squareSample + (1.0 - phase_increment) * y1;
    y1 = triangleSample;

    // compute noise
    sample_t noiseSample = noise.next();

    // update phase
    phase += phase_increment; // + pm
    phase = phase > 1 ? phase - 1 : phase;

    // mix output
    return linearXFade4(triangleSample, squareSample, sawSample, noiseSample,
                        oscMix);
  }
};

template <typename sample_t> struct SubtractiveVoice {
  bool gate;
  bool active;
  Parameter<sample_t> frequency;
  sample_t gain;
  Parameter<sample_t> filterCutoff;
  Parameter<sample_t> filterQuality;
  Parameter<sample_t> oscMix;
  Parameter<sample_t> attackTime;
  Parameter<sample_t> releaseTime;
  MultiOscillator<sample_t> osc;
  Biquad<sample_t, sample_t> filter;
  ASREnvelope<sample_t> env;

  sample_t sampleRate = 48000.0;

  inline void setSampleRate(sample_t sampleRate) {
    sampleRate = sampleRate;
    filter.lowpass(filterCutoff.value, filterCutoff.value, sampleRate);
  }

  inline const sample_t next() {
    auto nextFrequency = frequency.next();
    auto nextOscMix = oscMix.next();
    osc.frequency = nextFrequency;
    osc.oscMix = nextOscMix;
    auto out = osc.next();
    env.set(attackTime.next(), releaseTime.next(), sampleRate);
    env.setGate(gate);
    if (env.stage == ASREnvelope<sample_t>::Stage::OFF) {
      active = false;
    }
    filter.lowpass(filterCutoff.next(), filterCutoff.next(), sampleRate);
    return filter.next(out) * env.next();
  }
};

template <typename sample_t> struct SubtractiveSynthesizer {
  static const size_t MAX_VOICES = 16;
  size_t voiceIndex = 0;
  std::array<SubtractiveVoice<sample_t>, MAX_VOICES> voices;
  Parameter<sample_t> gain;
  std::array<sample_t, MAX_VOICES> notes;
  sample_t sampleRate = 48000;

  SubtractiveSynthesizer<sample_t>() {}

  inline const sample_t next() {
    sample_t out = 0;
    for (auto &voice : voices) {
      if (voice.active) {
        out += voice.next();
      }
    }
    return out * gain.next();
  }

  inline void note(const sample_t note, const sample_t velocity) {
    if (velocity > 0) {
      voices[voiceIndex].gate = true;
      voices[voiceIndex].active = true;
      voices[voiceIndex].gain = (velocity / 127.0);
      voices[voiceIndex].frequency.set(mtof(note), 5, sampleRate);
      notes[voiceIndex] = note;
      voiceIndex = voiceIndex < (MAX_VOICES - 1) ? (voiceIndex + 1) : 0;
    } else {
      for (size_t i = 0; i < MAX_VOICES; ++i) {
        if (notes[i] == note) {
          voices[i].gate = false;
        }
      }
    }
  }

  inline void setGain(sample_t value) { gain.set(value, 5, sampleRate); }
  inline void setFilterCutoff(sample_t value) {
    for (auto &voice : voices) {
      voice.filterCutoff.set(value, 5, sampleRate);
    }
  }
  inline void setFilterQuality(sample_t value) {
    for (auto &voice : voices) {
      voice.filterQuality.set(value, 5, sampleRate);
    }
  }
  inline void setSoundSource(sample_t value) {
    for (auto &voice : voices) {
      voice.oscMix.set(value, 5, sampleRate);
    }
  }
  inline void setAttackTime(sample_t value) {
    for (auto &voice : voices) {
      voice.attackTime.set(value, 5, sampleRate);
    }
  }
  inline void setReleaseTime(sample_t value) {
    for (auto &voice : voices) {
      voice.releaseTime.set(value, 5, sampleRate);
    }
  }
};

template <typename sample_t> struct NoteEvent {
  sample_t note;
  sample_t velocity;
};

template <typename sample_t> struct ParameterChangeEvent {
  enum ParameterType {
    GAIN,
    SOUND_SOURCE,
    FILTER_CUTOFF,
    FILTER_QUALITY,
    ATTACK_TIME,
    RELEASE_TIME
  } type;
  sample_t value;
};

enum SynthesizerType { SUBTRACTIVE, PHYSICAL_MODEL };

template <typename sample_t> struct SynthesizerEvent {
  enum EventType { NOTE, PARAMETER_CHANGE, SYNTHESIZER_CHANGE } type;
  union uEventData {
    NoteEvent<sample_t> note;
    ParameterChangeEvent<sample_t> paramChange;
    SynthesizerType newSynthType;
  } data;
};
template <typename sample_t> struct Synthesizer {
  SynthesizerType type;
  union uSynthesizer {
    SubtractiveSynthesizer<sample_t> subtractive;
    KarplusStrongSynthesizer<sample_t> physicalModel;
    uSynthesizer(const SubtractiveSynthesizer<sample_t> &s) : subtractive(s) {}
    uSynthesizer(const KarplusStrongSynthesizer<sample_t> &s)
        : physicalModel(s) {}
  } object;

  rigtorp::SPSCQueue<SynthesizerEvent<sample_t>> eventQueue =
      rigtorp::SPSCQueue<SynthesizerEvent<sample_t>>(20);

  Synthesizer<sample_t>(const SubtractiveSynthesizer<sample_t> &s)
      : object(s), type(SUBTRACTIVE) {}
  Synthesizer<sample_t>(const KarplusStrongSynthesizer<sample_t> &s)
      : object(s), type(PHYSICAL_MODEL) {}

  inline const sample_t next() {
    consumeMessagesFromQueue();
    switch (type) {
    case SUBTRACTIVE: {
      return object.subtractive.next();
      break;
    }
    case PHYSICAL_MODEL: {
      return object.physicalModel.next();
      break;
    }
    }
    return 0;
  }

  inline void consumeMessagesFromQueue() {
    while (!eventQueue.empty()) {
      auto event = eventQueue.front();

      switch (event->type) {
      case SynthesizerEvent<sample_t>::SYNTHESIZER_CHANGE: {
        switch (event->data.newSynthType) {
        case SUBTRACTIVE: {
          type = SUBTRACTIVE;
          object.subtractive = SubtractiveSynthesizer<sample_t>();
          break;
        }
        case PHYSICAL_MODEL: {
          type = PHYSICAL_MODEL;
          object.physicalModel = KarplusStrongSynthesizer<sample_t>();
          break;
        }
        }
        break;
      }
      case SynthesizerEvent<sample_t>::NOTE: {
        setNote(event->data.note.note, event->data.note.velocity);
        break;
      }
      case SynthesizerEvent<sample_t>::PARAMETER_CHANGE: {
        auto value = event->data.paramChange.value;
        switch (event->data.paramChange.type) {
        case ParameterChangeEvent<sample_t>::GAIN: {
          setGain(value);
          break;
        }
        case ParameterChangeEvent<sample_t>::SOUND_SOURCE: {
          setSoundSource(value);
          break;
        }
        case ParameterChangeEvent<sample_t>::FILTER_CUTOFF: {
          setFilterCutoff(value);
          break;
        }
        case ParameterChangeEvent<sample_t>::FILTER_QUALITY: {
          setFilterQuality(value);
          break;
        }
        case ParameterChangeEvent<sample_t>::ATTACK_TIME: {
          setAttackTime(value);
          break;
        }
        case ParameterChangeEvent<sample_t>::RELEASE_TIME: {
          setReleaseTime(value);
          break;
        }
        }
        break;
      }
      }

      eventQueue.pop();
    }
  }

  inline void setSoundSource(sample_t value) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setSoundSource(value);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setSoundSource(value);
      break;
    }
    }
  }
  inline void setNote(sample_t note, sample_t velocity) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.note(note, velocity);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.note(note, velocity);
      break;
    }
    }
  }
  inline void setGain(sample_t value) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setGain(value);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setGain(value);
      break;
    }
    }
  }
  inline void setFilterCutoff(sample_t value) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setFilterCutoff(value);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setFilterCutoff(value);
      break;
    }
    }
  }
  inline void setFilterQuality(sample_t value) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setFilterQuality(value);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setFilterQuality(value);
      break;
    }
    }
  }
  inline void setAttackTime(sample_t value) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setAttackTime(value);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setAttackTime(value);
      break;
    }
    }
  }
  inline void setReleaseTime(sample_t value) {
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setReleaseTime(value);
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setReleaseTime(value);
      break;
    }
    }
  }
};
