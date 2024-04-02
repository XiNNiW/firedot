#pragma once
#include "SDL_log.h"
#include "synthesis_frequency_modulation.h"
#include "synthesis_mixing.h"
#include "synthesis_parameter.h"
#include "synthesis_physical_modeling.h"
#include "synthesis_sampling.h"
#include "synthesis_subtractive.h"
#include "synthesizer_settings.h"
#include <algae.h>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <rigtorp/SPSCQueue.h>

template <typename sample_t> struct PitchBendEvent {
  sample_t note = 0;
  sample_t destinationNote = 0;
};
template <typename sample_t> struct ParameterChangeEvent {
  ContinuousParameterType type;
  sample_t value;
};
template <typename sample_t> struct GateEvent {
  sample_t value;
};
template <typename sample_t> struct SynthesizerEvent {
  enum EventType {
    GATE,
    PARAMETER_CHANGE,
    SYNTHESIZER_CHANGE,
    PITCH_BEND
  } type;
  union uEventData {
    GateEvent<sample_t> gate;
    ParameterChangeEvent<sample_t> paramChange;
    SynthesizerType newSynthType;
    PitchBendEvent<sample_t> pitchBend;
    uEventData(const GateEvent<sample_t> &n) : gate(n) {}
    uEventData(const ParameterChangeEvent<sample_t> &p) : paramChange(p) {}
    uEventData(const SynthesizerType &s) : newSynthType(s) {}
    uEventData(const PitchBendEvent<sample_t> &b) : pitchBend(b) {}
  } data;
  SynthesizerEvent<sample_t>() {}
  SynthesizerEvent<sample_t>(const GateEvent<sample_t> &gateEvent)
      : data(gateEvent), type(GATE) {}
  SynthesizerEvent<sample_t>(
      const ParameterChangeEvent<sample_t> &parameterChangeEvent)
      : data(parameterChangeEvent), type(PARAMETER_CHANGE) {}
  SynthesizerEvent<sample_t>(const SynthesizerType &synthType)
      : data(synthType), type(SYNTHESIZER_CHANGE) {}
  SynthesizerEvent<sample_t>(const PitchBendEvent<sample_t> &bend)
      : data(bend), type(PITCH_BEND) {}
};

template <typename sample_t> struct Synthesizer {
  const float MIN_FREQUENCY = mtof(24);
  const float MAX_FREQUENCY = mtof(40 + 6 * 5);
  std::atomic<sample_t> frequency = 440;
  std::atomic<sample_t> gain = 1;
  std::atomic<sample_t> filterCutoff = 1;
  std::atomic<sample_t> filterQuality = 0.3;
  std::atomic<sample_t> soundSource = 0;
  std::atomic<sample_t> attackTime = 0;
  std::atomic<sample_t> releaseTime = 1;
  std::atomic<sample_t> octave = 0;

  sample_t sampleRate = 48000;
  SampleBank<sample_t> *sampleBank = NULL;
  Arena *delayTimeArena = NULL;
  KarplusStrongSynthesizer<sample_t> physicalModel;

  SynthesizerType type;
  union uSynthesizer {
    SubtractiveDrumSynth<sample_t> subtractiveDrumSynth;
    SubtractiveSynthesizer<sample_t> subtractive;
    FMSynthesizer<sample_t> fm;
    Sampler<sample_t> sampler;
    uSynthesizer(const SubtractiveDrumSynth<sample_t> &s)
        : subtractiveDrumSynth(s) {}
    uSynthesizer(const SubtractiveSynthesizer<sample_t> &s) : subtractive(s) {}
    // uSynthesizer(const KarplusStrongSynthesizer<sample_t> &s)
    //     : physicalModel(s) {}
    uSynthesizer(const FMSynthesizer<sample_t> &s) : fm(s) {}
    uSynthesizer(const Sampler<sample_t> &s) : sampler(s) {}
  } object;

  rigtorp::SPSCQueue<SynthesizerEvent<sample_t>> eventQueue =
      rigtorp::SPSCQueue<SynthesizerEvent<sample_t>>(20);

  Synthesizer<sample_t>(SampleBank<sample_t> *bank, Arena *_delayTimeArena,
                        const SynthesizerSettings &synthesizerSettings)
      : sampleBank(bank), delayTimeArena(_delayTimeArena),
        object(SubtractiveDrumSynth<sample_t>()),
        type(synthesizerSettings.synthType),
        physicalModel(KarplusStrongSynthesizer<sample_t>(_delayTimeArena)) {
    gain = synthesizerSettings.gain;
    octave = synthesizerSettings.octave;
    filterCutoff = synthesizerSettings.filterCutoff;
    filterQuality = synthesizerSettings.filterQuality;
    soundSource = synthesizerSettings.soundSource;
    attackTime = synthesizerSettings.attack;
    releaseTime = synthesizerSettings.release;
  }

  inline const void process(sample_t *block, const size_t &blockSize) {
    consumeMessagesFromQueue();
    for (size_t i = 0; i < blockSize; i++) {
      block[i] = computeNextSample();
    }
  }

  inline const sample_t next() {
    consumeMessagesFromQueue();
    return computeNextSample();
  }

  inline const sample_t computeNextSample() {
    static const sample_t octaveMultiples[6] = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
    auto registerMultiplier = octaveMultiples[int(
        algae::dsp::math::clamp<sample_t>(octave * 6, 0.0, 5.0))];
    auto nextFrequency = frequency * registerMultiplier;
    switch (type) {

    case SUBTRACTIVE_DRUM_SYNTH: {
      object.subtractiveDrumSynth.setFrequency(nextFrequency);
      object.subtractiveDrumSynth.setGain(gain);
      object.subtractiveDrumSynth.setFilterCutoff(filterCutoff);
      object.subtractiveDrumSynth.setFilterQuality(filterQuality);
      object.subtractiveDrumSynth.setSoundSource(soundSource);
      object.subtractiveDrumSynth.setAttackTime(attackTime);
      object.subtractiveDrumSynth.setReleaseTime(releaseTime);
      return object.subtractiveDrumSynth.next();
      break;
    }
    case SUBTRACTIVE: {
      object.subtractive.setFrequency(nextFrequency);
      object.subtractive.setGain(gain);
      object.subtractive.setFilterCutoff(filterCutoff);
      object.subtractive.setFilterQuality(filterQuality);
      object.subtractive.setSoundSource(soundSource);
      object.subtractive.setAttackTime(attackTime);
      object.subtractive.setReleaseTime(releaseTime);
      return object.subtractive.next();
      break;
    }
    case PHYSICAL_MODEL: {
      physicalModel.setFrequency(nextFrequency);
      physicalModel.setGain(gain);
      physicalModel.setFilterCutoff(filterCutoff);
      physicalModel.setFilterQuality(filterQuality);
      physicalModel.setSoundSource(soundSource);
      physicalModel.setAttackTime(attackTime);
      physicalModel.setReleaseTime(releaseTime);
      return physicalModel.next();
      break;
    }
    case FREQUENCY_MODULATION: {
      object.fm.setFrequency(nextFrequency);
      object.fm.setGain(gain);
      object.fm.setFilterCutoff(filterCutoff);
      object.fm.setFilterQuality(filterQuality);
      object.fm.setSoundSource(soundSource);
      object.fm.setAttackTime(attackTime);
      object.fm.setReleaseTime(releaseTime);
      return object.fm.next();
      break;
    }
    case SAMPLER: {
      object.sampler.setFrequency(nextFrequency);
      object.sampler.setGain(gain);
      object.sampler.setFilterCutoff(filterCutoff);
      object.sampler.setFilterQuality(filterQuality);
      object.sampler.setSoundSource(soundSource);
      object.sampler.setAttackTime(attackTime);
      object.sampler.setReleaseTime(releaseTime);
      return object.sampler.next();
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
        case SUBTRACTIVE_DRUM_SYNTH: {
          type = SUBTRACTIVE_DRUM_SYNTH;
          object.subtractiveDrumSynth = SubtractiveDrumSynth<sample_t>();
          break;
        }
        case SUBTRACTIVE: {
          type = SUBTRACTIVE;
          object.subtractive = SubtractiveSynthesizer<sample_t>();
          break;
        }
        case PHYSICAL_MODEL: {
          type = PHYSICAL_MODEL;
          //  delayTimeArena->clear();
          //  physicalModel =
          //      KarplusStrongSynthesizer<sample_t>(delayTimeArena);
          break;
        }
        case FREQUENCY_MODULATION: {
          type = FREQUENCY_MODULATION;
          object.fm = FMSynthesizer<sample_t>();
          break;
        }
        case SAMPLER: {
          type = SAMPLER;
          object.sampler = Sampler<sample_t>(sampleBank);
          break;
        }
        }
        break;
      }
      case SynthesizerEvent<sample_t>::GATE: {
        switch (type) {
        case SUBTRACTIVE_DRUM_SYNTH:
          object.subtractiveDrumSynth.setGate(event.data.gate.value);
          break;
        case SUBTRACTIVE:
          object.subtractive.setGate(event.data.gate.value);
          break;
        case PHYSICAL_MODEL:
          physicalModel.setGate(event.data.gate.value);
          break;
        case FREQUENCY_MODULATION:
          object.fm.setGate(event.data.gate.value);
          break;
        case SAMPLER:
          object.sampler.setGate(event.data.gate.value);
          break;
        }
        break;
      }
      case SynthesizerEvent<sample_t>::PARAMETER_CHANGE: {
        auto value = event.data.paramChange.value;
        switch (event.data.paramChange.type) {
        case FREQUENCY: {
          frequency = value;
          break;
        }
        case GAIN: {
          gain = value;
          break;
        }
        case SOUND_SOURCE: {
          soundSource = value;
          break;
        }
        case FILTER_CUTOFF: {
          filterCutoff = value;
          break;
        }
        case FILTER_QUALITY: {
          filterQuality = value;
          break;
        }
        case ATTACK_TIME: {
          attackTime = value;
          break;
        }
        case RELEASE_TIME: {
          releaseTime = value;
          break;
        }
        case OCTAVE: {
          octave = value;
          break;
        }
        }
        break;
      }
      case SynthesizerEvent<sample_t>::PITCH_BEND: {
        switch (type) {
        case SUBTRACTIVE_DRUM_SYNTH: {
          object.subtractiveDrumSynth.bendNote(
              event.data.pitchBend.note, event.data.pitchBend.destinationNote);
          break;
        }
        case SUBTRACTIVE: {
          object.subtractive.bendNote(event.data.pitchBend.note,
                                      event.data.pitchBend.destinationNote);
          break;
        }
        case PHYSICAL_MODEL: {
          physicalModel.bendNote(event.data.pitchBend.note,
                                 event.data.pitchBend.destinationNote);
          break;
        }
        case FREQUENCY_MODULATION: {
          object.fm.bendNote(event.data.pitchBend.note,
                             event.data.pitchBend.destinationNote);
          break;
        }
        case SAMPLER: {
          object.sampler.bendNote(event.data.pitchBend.note,
                                  event.data.pitchBend.destinationNote);
          break;
        }
        }
        break;
      }
      }
    }
  }

  inline void pushParameterChangeEvent(ContinuousParameterType type,
                                       sample_t value) {
    eventQueue.push(
        ParameterChangeEvent<sample_t>{.type = type, .value = value});
  }

  inline void pushGateEvent(MomentaryParameterType type, sample_t value) {
    eventQueue.push(GateEvent<sample_t>{.value = value});
  }

  inline void setSynthType(SynthesizerType type) {
    eventQueue.push(SynthesizerEvent<sample_t>(type));
  }

  inline SynthesizerType getSynthType() const { return type; }

  inline void setFrequency(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = FREQUENCY, .value = value}));
  }

  inline void setSoundSource(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = SOUND_SOURCE, .value = value}));
  }

  inline void setGain(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = GAIN, .value = value}));
  }

  inline void setFilterCutoff(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = FILTER_CUTOFF, .value = value}));
  }

  inline void setFilterQuality(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(ParameterChangeEvent<sample_t>{
        .type = FILTER_QUALITY, .value = value}));
  }

  inline void setAttackTime(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = ATTACK_TIME, .value = value}));
  }

  inline void setReleaseTime(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = RELEASE_TIME, .value = value}));
  }

  inline void setOctave(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = OCTAVE, .value = value}));
  }

  inline const sample_t
  getParameter(const ContinuousParameterType parameterType) const {

    // TODO this also needs better threading consideration
    switch (parameterType) {
    case FREQUENCY:
      return (frequency - MIN_FREQUENCY) /
             MAX_FREQUENCY; // noteMap.getNormalizedValue(frequency.smoothedValue);
    case GAIN:
      return gain;
    case SOUND_SOURCE:
      return soundSource;
    case FILTER_CUTOFF:
      return filterCutoff;
    case FILTER_QUALITY:
      return filterQuality;
    case ATTACK_TIME:
      return attackTime;
    case RELEASE_TIME:
      return releaseTime;
      break;
    case OCTAVE:
      return octave;
    case _SIZE_ContinuousParameterType:
      break;
    }
    return 0;
  }
};
