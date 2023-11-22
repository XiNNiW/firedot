#pragma once

#include <algae.h>
#include <atomic>

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
