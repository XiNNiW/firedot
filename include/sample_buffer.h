#pragma once

#include "arena.h"
#include <cstddef>
template <typename sample_t> struct SampleBuffer {
  sample_t sampleRate;
  size_t numChannels;
  sample_t *buffer;
  size_t bufferSize;
  SampleBuffer(Arena *arena, const sample_t _sampleRate,
               const size_t _bufferSize, const size_t _numChannels)
      : sampleRate(_sampleRate), bufferSize(_bufferSize),
        numChannels(_numChannels),
        buffer(arena->pushArray<sample_t>(_bufferSize)) {}
  inline sample_t read(float phase) {
    sample_t readPosition = phase * (bufferSize - 1);
    int r1 = floor(readPosition);
    int r2 = (r1 + 1) % bufferSize;
    sample_t mantissa = readPosition - sample_t(r1);

    return lerp(buffer[r1], buffer[r2], mantissa);
  }
};
