#pragma once

#include "sensor.h"
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
  RELEASE_TIME
};

static const size_t NUM_PARAMETER_TYPES = 7;
static_assert(RELEASE_TIME == NUM_PARAMETER_TYPES - 1,
              "synth type table and enum must agree");
static const ContinuousParameterType ParameterTypes[NUM_PARAMETER_TYPES] = {
    FREQUENCY,      GAIN,        SOUND_SOURCE, FILTER_CUTOFF,
    FILTER_QUALITY, ATTACK_TIME, RELEASE_TIME};

static const char *getDisplayName(ContinuousParameterType type) {
  static const char *ParameterTypeDisplayNames[NUM_PARAMETER_TYPES] = {
      "frequency",      "gain",        "sound source", "filter cutoff",
      "filter quality", "attack time", "release time"};
  return ParameterTypeDisplayNames[static_cast<int>(type)];
}
enum MomentaryParameterType { GATE };
static const size_t NUM_MOMENTARY_PARAMETER_TYPES = 1;
static_assert(GATE == NUM_MOMENTARY_PARAMETER_TYPES - 1,
              "synth type table and enum must agree");
static const MomentaryParameterType
    MomentaryParameterTypes[NUM_MOMENTARY_PARAMETER_TYPES] = {GATE};

static const char *getDisplayName(MomentaryParameterType type) {
  static const char
      *MomentaryParameterTypeDisplayNames[NUM_MOMENTARY_PARAMETER_TYPES] = {
          "gate"};
  return MomentaryParameterTypeDisplayNames[static_cast<int>(type)];
}

template <typename sample_t> struct NoteEvent {
  sample_t frequency = 440;
  sample_t gate = 0;
};

template <typename sample_t> struct ParameterChangeEvent {
  ContinuousParameterType type;
  sample_t value;
};

using algae::dsp::filter::SmoothParameter;

template <typename sample_t> struct Parameter {
  sample_t value = 0;
  std::atomic<sample_t> smoothedValue;
  SmoothParameter<sample_t> smoothingFilter;
  Parameter<sample_t>(const Parameter<float> &p)
      : smoothedValue(p.smoothedValue.load()), value(value),
        smoothingFilter(p.smoothingFilter) {}

  Parameter<sample_t>() {}
  Parameter<sample_t>(float initialValue) : value(initialValue) {}
  void operator=(const Parameter<sample_t> other) {
    value = other.value;
    smoothedValue = other.smoothedValue.load();
    smoothingFilter = other.smoothingFilter;
  }
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
