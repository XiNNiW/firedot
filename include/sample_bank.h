#pragma once

#include "arena.h"
#include "sample_buffer.h"
#include "sample_load.h"
#include <string>
template <typename sample_t> struct SampleBank {
  static constexpr size_t MAX_BANK_SIZE = 16;
  Arena *arena = NULL;
  SampleBuffer<sample_t> *buffers[MAX_BANK_SIZE];
  size_t size = 0;
  SampleBank(Arena *sampleArena) : arena(sampleArena) {}
  void clear() {
    buffers.clear();
    arena->clear();
    size = 0;
  }
  bool loadSample(const std::string &path) {
    if (size == MAX_BANK_SIZE)
      return false;
    auto sampleBuffer = LoadWAVSampleAsMono(arena, path);
    if (sampleBuffer != NULL) {
      SDL_Log("Sample load %s", path.c_str());
      buffers[size++] = sampleBuffer;
      return true;
    } else {
      return false;
    }
  }
};
