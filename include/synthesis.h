#pragma once

#include <algae.h>
#include <atomic>
#include <cstddef>
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
  Parameter<sample_t>() {}
  Parameter<sample_t>(float initialValue) : value(initialValue) {}

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

template <typename sample_t> struct KarplusStringVoice {
  sample_t active = false;

  static const size_t MAX_DELAY_SAMPS =
      static_cast<size_t>((70.0 * 48000.0) / 1000.0);

  sample_t exciterMix = 0.5;
  sample_t allpassFilterGain = -0.5;
  Parameter<sample_t> frequency = Parameter<sample_t>(440);
  sample_t feedback = 0.99;
  sample_t b1Coefficient = 1.0;
  sample_t gain = 1.0;
  sample_t attackTime = 1;
  sample_t releaseTime = 1000;

  //
  ASREnvelope<sample_t> env;
  WhiteNoise<sample_t> exciterNoise;
  SinOscillator<sample_t, sample_t> exciterTone;
  ADEnvelope<sample_t> exciterEnvelope;
  Allpass2Comb<sample_t, MAX_DELAY_SAMPS> apf;
  InterpolatedDelay<sample_t, MAX_DELAY_SAMPS> delay;
  Onepole<sample_t, sample_t> inputFilter;
  Onezero<sample_t> lp;

  sample_t sampleRate = 48000;
  sample_t y1 = 0;
  sample_t h1 = 1.0;

  KarplusStringVoice<sample_t>() { init(); }
  KarplusStringVoice<sample_t>(const sample_t sr) : sampleRate(sr) { init(); }
  void init() {
    env.set(attackTime, releaseTime, sampleRate);
    inputFilter.lowpass(19000, sampleRate);
    exciterEnvelope.set(0.1, 1, sampleRate);
  }

  void setSampleRate(sample_t sr) { sampleRate = sr; }

  const inline sample_t next() {

    const sample_t freq = frequency.next();
    const sample_t dtime = (sampleRate / freq);
    const sample_t allpassDelayTimeSamples = (sampleRate / (h1 * freq));

    exciterTone.setFrequency(freq, sampleRate);
    sample_t tone = exciterTone.next();
    sample_t noise = exciterNoise.next();

    sample_t exciterLevel = exciterEnvelope.next();
    sample_t exciter = lerp<sample_t>(noise, tone, exciterLevel);
    exciter *= exciterLevel;
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

  inline void note(sample_t note, sample_t velocity) {

    if (velocity > 0) {
      SDL_Log("note on (%f, %f) for %d", note, velocity, voiceIndex);
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

  inline void setGain(sample_t value) { gain = value; }
  inline void setFilterCutoff(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    value = lerp<sample_t>(0.8, 1, value);
    for (auto &voice : voices) {
      voice.b1Coefficient = value;
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
    for (auto &voice : voices) {
      voice.attackTime = value;
    }
  }
  inline void setReleaseTime(sample_t value) {
    for (auto &voice : voices) {
      voice.releaseTime = value;
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
  sample_t oscMix = 0;
  sample_t phase_increment = 0;
  sample_t phase = 0;
  sample_t y1 = 0;

  inline void setFrequency(const sample_t frequency,
                           const sample_t sampleRate) {
    phase_increment =
        algae::dsp::oscillator::computePhaseIncrement(frequency, sampleRate);
  }

  inline const sample_t next() {

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
  static const size_t MAX_VOICES = 16;
  size_t voiceIndex = 0;
  SubtractiveVoice<sample_t> voices[MAX_VOICES];
  sample_t gain = 1;
  sample_t notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1,
                                -1, -1, -1, -1, -1, -1, -1, -1};
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

  inline void note(const sample_t note, const sample_t velocity) {
    if (velocity > 0) {
      SDL_Log("note on (%f, %f) for %d", note, velocity, voiceIndex);
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

  inline void setGain(sample_t value) { gain = value; }
  inline void setFilterCutoff(sample_t value) {
    value = lerp<sample_t>(500, 19000, value);
    for (auto &voice : voices) {
      voice.filterCutoff = value;
    }
  }
  inline void setFilterQuality(sample_t value) {
    for (auto &voice : voices) {
      voice.filterQuality = value;
    }
  }
  inline void setSoundSource(sample_t value) {
    for (auto &voice : voices) {
      voice.osc.oscMix = value;
    }
  }
  inline void setAttackTime(sample_t value) {
    for (auto &voice : voices) {
      voice.attackTime = value;
    }
  }
  inline void setReleaseTime(sample_t value) {
    for (auto &voice : voices) {
      voice.releaseTime = value;
    }
  }
};

template <typename sample_t> struct FMOperator {
  sample_t sampleRate = 48000;
  SinOscillator<sample_t, sample_t> osc;
  ASREnvelope<sample_t> env;
  sample_t freq = 440;
  sample_t last = 0;

  inline const sample_t next(sample_t pmod = 0) {
    osc.setFrequency(freq, sampleRate);
    last = env.next() * osc.next(pmod);
    return last;
  }

  inline void setFrequency(sample_t f) { freq = f; }
};

template <typename sample_t> struct FM4OpVoice {

  sample_t modMatrix[4][4] = {
      {0, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

  sample_t ratio1 = sample_t(1), ratio2 = sample_t(2), ratio3 = sample_t(3),
           ratio4 = sample_t(2.0 / 3.0);

  sample_t outputLevels[4] = {1, 0, 0, 0};

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

    op1.setFrequency(nextFrequency * ratio1);
    op2.setFrequency(nextFrequency * ratio2);
    op3.setFrequency(nextFrequency * ratio3);
    op4.setFrequency(nextFrequency * ratio4);

    sample_t mod1 =
        index * (op1.last * modMatrix[0][0] + op2.last * modMatrix[0][1] +
                 op3.last * modMatrix[0][2] + op4.last * modMatrix[0][3]);
    sample_t mod2 =
        index * (op1.last * modMatrix[1][0] + op2.last * modMatrix[1][1] +
                 op3.last * modMatrix[1][2] + op4.last * modMatrix[1][3]);
    sample_t mod3 =
        index * (op1.last * modMatrix[2][0] + op2.last * modMatrix[2][1] +
                 op3.last * modMatrix[2][2] + op4.last * modMatrix[2][3]);
    sample_t mod4 =
        index * (op1.last * modMatrix[3][0] + op2.last * modMatrix[3][1] +
                 op3.last * modMatrix[3][2] + op4.last * modMatrix[3][3]);
    sample_t out =
        op1.next(mod1) * outputLevels[0] + op2.next(mod2) * outputLevels[1] +
        op2.next(mod3) * outputLevels[2] + op4.next(mod4) * outputLevels[3];
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
  static const size_t MAX_VOICES = 16;
  size_t voiceIndex = 0;

  static const size_t NUM_RATIOS = 8;
  sample_t ratios[NUM_RATIOS][8] = {
      {1.0 / 64.0, 2.0, 1.0 / 16.0}, {3.0 / 32.0, 1.5, 2.0},
      {1.0 / 8.0, 1.0, 5.0 / 8.0},   {5.0 / 16.0, 1.0 / 2.0, 4.0},
      {0.5, 3.0 / 4.0, 1.75},        {5.0 / 8.0, 1.0 / 4.0, 7.0 / 16.0},
      {1.0, 5.0 / 32.0, 3.5},        {2.0, 1.0 / 32.0, 1.0}};

  static const size_t NUM_ALGS = 8;
  sample_t topologies[NUM_ALGS][4][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {1, 1, 1, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}},
  };
  FM4OpVoice<sample_t> voices[MAX_VOICES];
  sample_t gain = 1;
  sample_t notes[MAX_VOICES] = {-1, -1, -1, -1, -1, -1, -1, -1,
                                -1, -1, -1, -1, -1, -1, -1, -1};
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

  inline void note(const sample_t note, const sample_t velocity) {
    if (velocity > 0) {
      SDL_Log("note on (%f, %f) for %d", note, velocity, voiceIndex);
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

  inline void setGain(sample_t value) { gain = value; }
  inline void setFilterCutoff(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    // auto index = value;
    //  auto fb = lerp<sample_t>(0.0, 1.0, pow(1, value));
    for (auto &voice : voices) {
      // voice.modMatrix[0][0] = voice.modMatrix[1][1] = voice.modMatrix[2][2] =
      //     voice.modMatrix[3][3] = fb;
      voice.index = value;
    }
  }
  inline void setFilterQuality(sample_t value) {
    value = clamp<sample_t>(value, 0, 1);
    int index = floor(value * (NUM_RATIOS - 1));
    auto ratioSet = ratios[index];

    for (auto &voice : voices) {
      voice.ratio2 = ratioSet[0];
      voice.ratio3 = ratioSet[1];
      voice.ratio4 = ratioSet[2];
    }
  }
  inline void setSoundSource(sample_t value) {
    // value = clamp<sample_t>(value, 0, 1);
    // sample_t amount = value * (NUM_ALGS - 1);
    // int index = floor(amount);
    // auto mantissa = amount - sample_t(index);
    // auto formation0 = topologies[index];
    // auto formation1 = topologies[(index + 1) % NUM_ALGS];
    // for (auto &voice : voices) {
    //   for (size_t i = 0; i < 4; ++i) {
    //     for (size_t j = 0; j < 4; ++j) {
    //       if (i == j) {
    //         voice.outputLevels[i] =
    //             lerp(formation0[i][j], formation1[i][j], mantissa);
    //       } else {
    //         voice.modMatrix[i][j] =
    //             lerp(formation0[i][j], formation1[i][j], mantissa);
    //       }
    //     }
    //   }
    // }
  }
  inline void setAttackTime(sample_t value) {
    for (auto &voice : voices) {
      // voice.setAttackTime(value);
    }
  }
  inline void setReleaseTime(sample_t value) {
    for (auto &voice : voices) {
      // voice.releaseTime = value;
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

enum SynthesizerType { SUBTRACTIVE, PHYSICAL_MODEL, FREQUENCY_MODULATION };

template <typename sample_t> struct SynthesizerEvent {
  enum EventType { NOTE, PARAMETER_CHANGE, SYNTHESIZER_CHANGE } type;
  union uEventData {
    NoteEvent<sample_t> note;
    ParameterChangeEvent<sample_t> paramChange;
    SynthesizerType newSynthType;
    uEventData(const NoteEvent<sample_t> &n) : note(n) {}
    uEventData(const ParameterChangeEvent<sample_t> &p) : paramChange(p) {}
    uEventData(const SynthesizerType &s) : newSynthType(s) {}
  } data;
  SynthesizerEvent<sample_t>() {}
  SynthesizerEvent<sample_t>(const NoteEvent<sample_t> &noteEvent)
      : data(noteEvent), type(NOTE) {}
  SynthesizerEvent<sample_t>(
      const ParameterChangeEvent<sample_t> &parameterChangeEvent)
      : data(parameterChangeEvent), type(PARAMETER_CHANGE) {}
  SynthesizerEvent<sample_t>(const SynthesizerType &synthType)
      : data(synthType), type(SYNTHESIZER_CHANGE) {}
};

template <typename sample_t> struct Synthesizer {
  Parameter<sample_t> gain = Parameter<sample_t>(1);
  Parameter<sample_t> filterCutoff = Parameter<sample_t>(17000);
  Parameter<sample_t> filterQuality = Parameter<sample_t>(0.3);
  Parameter<sample_t> soundSource = Parameter<sample_t>(0);
  Parameter<sample_t> attackTime = Parameter<sample_t>(10);
  Parameter<sample_t> releaseTime = Parameter<sample_t>(1000);
  sample_t sampleRate = 48000;
  SynthesizerType type;
  union uSynthesizer {
    SubtractiveSynthesizer<sample_t> subtractive;
    KarplusStrongSynthesizer<sample_t> physicalModel;
    FMSynthesizer<sample_t> fm;
    uSynthesizer(const SubtractiveSynthesizer<sample_t> &s) : subtractive(s) {}
    uSynthesizer(const KarplusStrongSynthesizer<sample_t> &s)
        : physicalModel(s) {}
    uSynthesizer(const FMSynthesizer<sample_t> &s) : fm(s) {}
  } object;

  rigtorp::SPSCQueue<SynthesizerEvent<sample_t>> eventQueue =
      rigtorp::SPSCQueue<SynthesizerEvent<sample_t>>(20);

  Synthesizer<sample_t>(const SubtractiveSynthesizer<sample_t> &s)
      : object(s), type(SUBTRACTIVE) {}
  Synthesizer<sample_t>(const KarplusStrongSynthesizer<sample_t> &s)
      : object(s), type(PHYSICAL_MODEL) {}
  Synthesizer<sample_t>(const FMSynthesizer<sample_t> &s)
      : object(s), type(FREQUENCY_MODULATION) {}

  inline const void process(sample_t *block, const size_t &blockSize) {
    consumeMessagesFromQueue();
    for (size_t i = 0; i < blockSize; i++) {
      block[i] = computeNextSample();
    }
  }

  inline const sample_t next() {
    // SDL_LogInfo(0, "sample tick");
    consumeMessagesFromQueue();
    return computeNextSample();
  }

  inline const sample_t computeNextSample() {
    auto nextGain = gain.next();
    auto nextFilterCutoff = filterCutoff.next();
    auto nextFilterQuality = filterQuality.next();
    auto nextSoundSource = soundSource.next();
    auto nextAttackTime = attackTime.next();
    auto nextReleaseTime = releaseTime.next();
    switch (type) {
    case SUBTRACTIVE: {
      object.subtractive.setGain(nextGain);
      object.subtractive.setFilterCutoff(nextFilterCutoff);
      object.subtractive.setFilterQuality(nextFilterQuality);
      object.subtractive.setSoundSource(nextSoundSource);
      object.subtractive.setAttackTime(nextAttackTime);
      object.subtractive.setReleaseTime(nextReleaseTime);
      return object.subtractive.next();
      break;
    }
    case PHYSICAL_MODEL: {
      object.physicalModel.setGain(nextGain);
      object.physicalModel.setFilterCutoff(nextFilterCutoff);
      object.physicalModel.setFilterQuality(nextFilterQuality);
      object.physicalModel.setSoundSource(nextSoundSource);
      object.physicalModel.setAttackTime(nextAttackTime);
      object.physicalModel.setReleaseTime(nextReleaseTime);
      return object.physicalModel.next();
      break;
    }
    case FREQUENCY_MODULATION: {
      object.fm.setGain(nextGain);
      object.fm.setFilterCutoff(nextFilterCutoff);
      object.fm.setFilterQuality(nextFilterQuality);
      object.fm.setSoundSource(nextSoundSource);
      object.fm.setAttackTime(nextAttackTime);
      object.fm.setReleaseTime(nextReleaseTime);
      return object.fm.next();
      break;
    }
    }
    return 0;
  }

  inline void consumeMessagesFromQueue() {
    while (!eventQueue.empty()) {
      SynthesizerEvent<sample_t> event = *eventQueue.front();
      eventQueue.pop();

      switch (event.type) {
      case SynthesizerEvent<sample_t>::SYNTHESIZER_CHANGE: {
        switch (event.data.newSynthType) {
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
        case FREQUENCY_MODULATION: {
          type = FREQUENCY_MODULATION;
          object.fm = FMSynthesizer<sample_t>();
          break;
        }
        }
        break;
      }
      case SynthesizerEvent<sample_t>::NOTE: {
        switch (type) {
        case SUBTRACTIVE:
          object.subtractive.note(event.data.note.note,
                                  event.data.note.velocity);

          break;
        case PHYSICAL_MODEL:
          object.physicalModel.note(event.data.note.note,
                                    event.data.note.velocity);
          break;
        case FREQUENCY_MODULATION:
          object.fm.note(event.data.note.note, event.data.note.velocity);
          break;
        }

        break;
      }
      case SynthesizerEvent<sample_t>::PARAMETER_CHANGE: {
        auto value = event.data.paramChange.value;
        switch (event.data.paramChange.type) {
        case ParameterChangeEvent<sample_t>::GAIN: {
          gain.set(value, 32, sampleRate);
          break;
        }
        case ParameterChangeEvent<sample_t>::SOUND_SOURCE: {
          soundSource.set(value, 32, sampleRate);
          break;
        }
        case ParameterChangeEvent<sample_t>::FILTER_CUTOFF: {
          filterCutoff.set(value, 32, sampleRate);
          break;
        }
        case ParameterChangeEvent<sample_t>::FILTER_QUALITY: {
          filterQuality.set(value, 32, sampleRate);
          break;
        }
        case ParameterChangeEvent<sample_t>::ATTACK_TIME: {
          attackTime.set(value, 32, sampleRate);
          break;
        }
        case ParameterChangeEvent<sample_t>::RELEASE_TIME: {
          releaseTime.set(value, 32, sampleRate);
          break;
        }
        }
        break;
      }
      }
    }
  }

  inline void setSynthType(SynthesizerType type) {
    eventQueue.push(SynthesizerEvent<sample_t>(type));
  }
  inline void setSoundSource(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = ParameterChangeEvent<sample_t>::SOUND_SOURCE, .value = value}));
  }
  inline void note(sample_t note, sample_t velocity) {
    auto event = SynthesizerEvent<sample_t>(
        NoteEvent<sample_t>{.note = note, .velocity = velocity});

    eventQueue.push(event);
  }
  inline void setGain(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = ParameterChangeEvent<sample_t>::GAIN, .value = value}));
  }

  inline void setFilterCutoff(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = ParameterChangeEvent<sample_t>::FILTER_CUTOFF,
        .value = value}));
  }
  inline void setFilterQuality(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = ParameterChangeEvent<sample_t>::FILTER_QUALITY,
        .value = value}));
  }
  inline void setAttackTime(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = ParameterChangeEvent<sample_t>::ATTACK_TIME, .value = value}));
  }
  inline void setReleaseTime(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = ParameterChangeEvent<sample_t>::RELEASE_TIME, .value = value}));
  }
};
