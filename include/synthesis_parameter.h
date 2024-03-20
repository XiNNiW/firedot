#pragma once

#include "synthesis_type.h"
#include <algae.h>
#include <atomic>
#include <cstddef>
enum ContinuousParameterType {
  FREQUENCY,
  GAIN,
  SOUND_SOURCE,
  FILTER_CUTOFF,
  FILTER_QUALITY,
  ATTACK_TIME,
  RELEASE_TIME,
  REGISTER,
  _SIZE_ContinuousParameterType
};

static const size_t NUM_PARAMETER_TYPES = _SIZE_ContinuousParameterType;
static_assert(REGISTER == NUM_PARAMETER_TYPES - 1,
              "synth type table and enum must agree");
static const ContinuousParameterType ParameterTypes[NUM_PARAMETER_TYPES] = {
    FREQUENCY,      GAIN,        SOUND_SOURCE, FILTER_CUTOFF,
    FILTER_QUALITY, ATTACK_TIME, RELEASE_TIME, REGISTER};

static const char *getDisplayNameForSubtractiveDrumParameter(
    ContinuousParameterType parameterType) {
  switch (parameterType) {
  case FREQUENCY:
    return "frequency";
  case GAIN:
    return "gain";
  case SOUND_SOURCE:
    return "drum type";
  case FILTER_CUTOFF:
    return "filter cutoff";
  case FILTER_QUALITY:
    return "filter quality";
  case ATTACK_TIME:
    return "attack time";
  case RELEASE_TIME:
    return "release time";
    break;
  case REGISTER:
    return "octave";
    break;
  case _SIZE_ContinuousParameterType:
    break;
  }
  return "";
}
static const char *
getDisplayNameForSubtractiveParameter(ContinuousParameterType parameterType) {
  switch (parameterType) {
  case FREQUENCY:
    return "frequency";
  case GAIN:
    return "gain";
  case SOUND_SOURCE:
    return "waveform";
  case FILTER_CUTOFF:
    return "filter cutoff";
  case FILTER_QUALITY:
    return "filter quality";
  case ATTACK_TIME:
    return "attack time";
  case RELEASE_TIME:
    return "release time";
    break;
  case REGISTER:
    return "octave";
    break;
  case _SIZE_ContinuousParameterType:
    break;
  }
  return "";
}
static const char *
getDisplayNameForPhysicalModelParameter(ContinuousParameterType parameterType) {
  switch (parameterType) {
  case FREQUENCY:
    return "frequency";
  case GAIN:
    return "gain";
  case SOUND_SOURCE:
    return "strike";
  case FILTER_CUTOFF:
    return "brightness";
  case FILTER_QUALITY:
    return "inharmonic";
  case ATTACK_TIME:
    return "attack time";
  case RELEASE_TIME:
    return "release time";
    break;
  case REGISTER:
    return "octave";
    break;
  case _SIZE_ContinuousParameterType:
    break;
  }
  return "";
}
static const char *getDisplayNameForFrequencyModulationParameter(
    ContinuousParameterType parameterType) {
  switch (parameterType) {
  case FREQUENCY:
    return "frequency";
  case GAIN:
    return "gain";
  case SOUND_SOURCE:
    return "mod. algorithm";
  case FILTER_CUTOFF:
    return "mod. index";
  case FILTER_QUALITY:
    return "freq. ratios";
  case ATTACK_TIME:
    return "attack time";
  case RELEASE_TIME:
    return "release time";
    break;
  case REGISTER:
    return "octave";
    break;
  case _SIZE_ContinuousParameterType:
    break;
  }
  return "";
}

static const char *
getDisplayNameForSamplerParameter(ContinuousParameterType parameterType) {
  switch (parameterType) {
  case FREQUENCY:
    return "frequency";
  case GAIN:
    return "gain";
  case SOUND_SOURCE:
    return "sample";
  case FILTER_CUTOFF:
    return "filter cutoff";
  case FILTER_QUALITY:
    return "filter quality";
  case ATTACK_TIME:
    return "attack time";
  case RELEASE_TIME:
    return "release time";
    break;
  case REGISTER:
    return "octave";
    break;
  case _SIZE_ContinuousParameterType:
    break;
  }
  return "";
}
static const char *getDisplayName(ContinuousParameterType parameterType,
                                  SynthesizerType synthType) {
  switch (synthType) {

  case SUBTRACTIVE_DRUM_SYNTH:
    return getDisplayNameForSubtractiveDrumParameter(parameterType);
    break;
  case SUBTRACTIVE:
    return getDisplayNameForSubtractiveParameter(parameterType);
    break;
  case PHYSICAL_MODEL:
    return getDisplayNameForPhysicalModelParameter(parameterType);
    break;
  case FREQUENCY_MODULATION:
    return getDisplayNameForFrequencyModulationParameter(parameterType);
    break;
  case SAMPLER:
    return getDisplayNameForSamplerParameter(parameterType);
    break;
    break;
  }
  return "";
}
enum MomentaryParameterType { GATE };
static const size_t NUM_MOMENTARY_PARAMETER_TYPES = 1;
static_assert(GATE == NUM_MOMENTARY_PARAMETER_TYPES - 1,
              "synth type table and enum must agree");
static const MomentaryParameterType
    MomentaryParameterTypes[NUM_MOMENTARY_PARAMETER_TYPES] = {GATE};

static const char *getDisplayName(MomentaryParameterType type) {
  switch (type) {
  case GATE:
    return "gate";
    break;
  }
  return "";
}

using algae::dsp::filter::SmoothParameter;

template <typename sample_t> struct Parameter {
  sample_t value = 0;
  sample_t smoothedValue;
  SmoothParameter<sample_t> smoothingFilter;
  Parameter<sample_t>() {}
  Parameter<sample_t>(float initialValue) : value(initialValue) {}
  inline const sample_t next() {
    smoothedValue = smoothingFilter.next(value);
    return smoothedValue;
  }
  void set(sample_t newValue, sample_t smoothingTimeMillis,
           sample_t sampleRate) {
    smoothingFilter.set(smoothingTimeMillis, sampleRate);
    value = newValue;
  }
};
