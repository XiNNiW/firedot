#pragma once
#include "SDL_log.h"
#include "synthesis_frequency_modulation.h"
#include "synthesis_mixing.h"
#include "synthesis_parameter.h"
#include "synthesis_physical_modeling.h"
#include "synthesis_sampling.h"
#include "synthesis_subtractive.h"
#include <algae.h>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <rigtorp/SPSCQueue.h>

template <typename sample_t> struct NoteEvent {
  sample_t note;
  sample_t velocity;
};

enum ParameterType {
  GAIN,
  SOUND_SOURCE,
  FILTER_CUTOFF,
  FILTER_QUALITY,
  ATTACK_TIME,
  RELEASE_TIME
};

static const size_t NUM_PARAMETER_TYPES = 6;
static_assert(RELEASE_TIME == NUM_PARAMETER_TYPES - 1,
              "synth type table and enum must agree");
static const ParameterType ParameterTypes[NUM_PARAMETER_TYPES] = {
    GAIN,           SOUND_SOURCE, FILTER_CUTOFF,
    FILTER_QUALITY, ATTACK_TIME,  RELEASE_TIME};
static const char *ParameterTypeDisplayNames[NUM_PARAMETER_TYPES] = {
    "gain",           "sound source", "filter cutoff",
    "filter quality", "attack time",  "release time"};

template <typename sample_t> struct ParameterChangeEvent {
  ParameterType type;
  sample_t value;
};

enum SynthesizerType {
  SUBTRACTIVE_DRUM_SYNTH,
  SUBTRACTIVE,
  PHYSICAL_MODEL,
  FREQUENCY_MODULATION,
  SAMPLER
};
static const size_t NUM_SYNTH_TYPES = 5;
static_assert(SAMPLER == NUM_SYNTH_TYPES - 1,
              "synth type table and enum must agree");
static const SynthesizerType SynthTypes[NUM_SYNTH_TYPES] = {
    SynthesizerType::SUBTRACTIVE_DRUM_SYNTH, SynthesizerType::SUBTRACTIVE,
    SynthesizerType::PHYSICAL_MODEL, SynthesizerType::FREQUENCY_MODULATION,
    SynthesizerType::SAMPLER};
static const char *SynthTypeDisplayNames[NUM_SYNTH_TYPES] = {
    "drum", "subtractive", "physical model", "frequency modulation", "sampler"};

template <typename sample_t> struct PitchBendEvent {
  sample_t note = 0;
  sample_t destinationNote = 0;
};

template <typename sample_t> struct SynthesizerEvent {
  enum EventType {
    NOTE,
    PARAMETER_CHANGE,
    SYNTHESIZER_CHANGE,
    PITCH_BEND
  } type;
  union uEventData {
    NoteEvent<sample_t> note;
    ParameterChangeEvent<sample_t> paramChange;
    SynthesizerType newSynthType;
    PitchBendEvent<sample_t> pitchBend;
    uEventData(const NoteEvent<sample_t> &n) : note(n) {}
    uEventData(const ParameterChangeEvent<sample_t> &p) : paramChange(p) {}
    uEventData(const SynthesizerType &s) : newSynthType(s) {}
    uEventData(const PitchBendEvent<sample_t> &b) : pitchBend(b) {}
  } data;
  SynthesizerEvent<sample_t>() {}
  SynthesizerEvent<sample_t>(const NoteEvent<sample_t> &noteEvent)
      : data(noteEvent), type(NOTE) {}
  SynthesizerEvent<sample_t>(
      const ParameterChangeEvent<sample_t> &parameterChangeEvent)
      : data(parameterChangeEvent), type(PARAMETER_CHANGE) {}
  SynthesizerEvent<sample_t>(const SynthesizerType &synthType)
      : data(synthType), type(SYNTHESIZER_CHANGE) {}
  SynthesizerEvent<sample_t>(const PitchBendEvent<sample_t> &bend)
      : data(bend), type(PITCH_BEND) {}
};

template <typename sample_t> struct Synthesizer {

  Parameter<sample_t> gain = Parameter<sample_t>(1);
  Parameter<sample_t> filterCutoff = Parameter<sample_t>(1);
  Parameter<sample_t> filterQuality = Parameter<sample_t>(0.3);
  Parameter<sample_t> soundSource = Parameter<sample_t>(0);
  Parameter<sample_t> attackTime = Parameter<sample_t>(0);
  Parameter<sample_t> releaseTime = Parameter<sample_t>(1);

  sample_t sampleRate = 48000;
  AudioSample *activeSample = NULL;

  SynthesizerType type;
  union uSynthesizer {
    SubtractiveDrumSynth<sample_t> subtractiveDrumSynth;
    SubtractiveSynthesizer<sample_t> subtractive;
    KarplusStrongSynthesizer<sample_t> physicalModel;
    FMSynthesizer<sample_t> fm;
    Sampler<sample_t> sampler;
    uSynthesizer(const SubtractiveDrumSynth<sample_t> &s)
        : subtractiveDrumSynth(s) {}
    uSynthesizer(const SubtractiveSynthesizer<sample_t> &s) : subtractive(s) {}
    uSynthesizer(const KarplusStrongSynthesizer<sample_t> &s)
        : physicalModel(s) {}
    uSynthesizer(const FMSynthesizer<sample_t> &s) : fm(s) {}
    uSynthesizer(const Sampler<sample_t> &s) : sampler(s) {}
  } object;

  rigtorp::SPSCQueue<SynthesizerEvent<sample_t>> eventQueue =
      rigtorp::SPSCQueue<SynthesizerEvent<sample_t>>(20);

  Synthesizer<sample_t>(const SubtractiveDrumSynth<sample_t> &s)
      : object(s), type(SUBTRACTIVE_DRUM_SYNTH) {}

  Synthesizer<sample_t>(const SubtractiveSynthesizer<sample_t> &s)
      : object(s), type(SUBTRACTIVE) {}
  Synthesizer<sample_t>(const KarplusStrongSynthesizer<sample_t> &s)
      : object(s), type(PHYSICAL_MODEL) {}
  Synthesizer<sample_t>(const FMSynthesizer<sample_t> &s)
      : object(s), type(FREQUENCY_MODULATION) {}
  Synthesizer<sample_t>(const Sampler<sample_t> &s)
      : object(s), type(SAMPLER) {}

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

    case SUBTRACTIVE_DRUM_SYNTH: {
      object.subtractiveDrumSynth.setGain(nextGain);
      object.subtractiveDrumSynth.setFilterCutoff(nextFilterCutoff);
      object.subtractiveDrumSynth.setFilterQuality(nextFilterQuality);
      object.subtractiveDrumSynth.setSoundSource(nextSoundSource);
      object.subtractiveDrumSynth.setAttackTime(nextAttackTime);
      object.subtractiveDrumSynth.setReleaseTime(nextReleaseTime);
      return object.subtractiveDrumSynth.next();
      break;
    }
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
    case SAMPLER: {
      object.sampler.setGain(nextGain);
      object.sampler.setFilterCutoff(nextFilterCutoff);
      object.sampler.setFilterQuality(nextFilterQuality);
      object.sampler.setSoundSource(nextSoundSource);
      object.sampler.setAttackTime(nextAttackTime);
      object.sampler.setReleaseTime(nextReleaseTime);
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
          object.physicalModel = KarplusStrongSynthesizer<sample_t>();
          break;
        }
        case FREQUENCY_MODULATION: {
          type = FREQUENCY_MODULATION;
          object.fm = FMSynthesizer<sample_t>();
          break;
        }
        case SAMPLER: {
          type = SAMPLER;
          object.sampler =
              Sampler<sample_t>(activeSample->buffer, activeSample->size);
          break;
        }
        }
        break;
      }
      case SynthesizerEvent<sample_t>::NOTE: {
        switch (type) {
        case SUBTRACTIVE_DRUM_SYNTH:
          object.subtractiveDrumSynth.note(event.data.note.note,
                                           event.data.note.velocity);
          break;
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
        case SAMPLER:
          object.sampler.note(event.data.note.note, event.data.note.velocity);
          break;
        }

        break;
      }
      case SynthesizerEvent<sample_t>::PARAMETER_CHANGE: {
        auto value = event.data.paramChange.value;
        switch (event.data.paramChange.type) {
        case GAIN: {
          gain.set(value, 32, sampleRate);
          break;
        }
        case SOUND_SOURCE: {
          soundSource.set(value, 32, sampleRate);
          break;
        }
        case FILTER_CUTOFF: {
          filterCutoff.set(value, 32, sampleRate);
          break;
        }
        case FILTER_QUALITY: {
          filterQuality.set(value, 32, sampleRate);
          break;
        }
        case ATTACK_TIME: {
          attackTime.set(value, 32, sampleRate);
          break;
        }
        case RELEASE_TIME: {
          releaseTime.set(value, 32, sampleRate);
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
          object.physicalModel.bendNote(event.data.pitchBend.note,
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

  inline void setSynthType(SynthesizerType type) {
    eventQueue.push(SynthesizerEvent<sample_t>(type));
  }
  inline SynthesizerType getSynthType() { return type; }
  inline void setSoundSource(sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = SOUND_SOURCE, .value = value}));
  }
  inline void note(sample_t note, sample_t velocity) {
    auto event = SynthesizerEvent<sample_t>(
        NoteEvent<sample_t>{.note = note, .velocity = velocity});

    eventQueue.push(event);
  }

  inline void pushParameterChangeEvent(ParameterType type, sample_t value) {
    eventQueue.push(SynthesizerEvent<sample_t>(
        ParameterChangeEvent<sample_t>{.type = type, .value = value}));
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

  inline void bendNote(sample_t note, sample_t destinationNote) {
    eventQueue.push(PitchBendEvent<sample_t>{
        .note = note, .destinationNote = destinationNote});
  }

  inline const sample_t getParameter(const ParameterType parameterType) {

    // TODO this also needs threading consideration
    switch (parameterType) {

    case GAIN:
      return gain.smoothedValue;
    case SOUND_SOURCE:
      return soundSource.smoothedValue;
    case FILTER_CUTOFF:
      return filterCutoff.smoothedValue;
    case FILTER_QUALITY:
      return filterQuality.smoothedValue;
    case ATTACK_TIME:
      return attackTime.smoothedValue;
    case RELEASE_TIME:
      return releaseTime.smoothedValue;
      break;
    }
    return 0;
  }
};
