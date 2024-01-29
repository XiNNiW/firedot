#pragma once

#include "SDL_audio.h"
#include "SDL_log.h"
#include "sample_buffer.h"
#include <string>
static inline SampleBuffer<float> *
LoadWAVSampleAsMono(Arena *arena, const std::string &samplePath) {

  SDL_AudioSpec wavSpec;
  Uint32 wavLength;
  Uint8 *wavBuffer;
  long bufferSize = 0;

  SDL_LoadWAV(samplePath.c_str(), &wavSpec, &wavBuffer, &wavLength);

  SampleBuffer<float> *sampleInfo = NULL;

  switch (wavSpec.format) {
  case AUDIO_S8: {
    const int bytesPerSample = sizeof(int8_t);
    SDL_Log("its a signed 8bit int!");
    break;
  }
  case AUDIO_U8: {
    const int bytesPerSample = sizeof(uint8_t);
    SDL_Log("its a unsigned 8bit int!");
    break;
  }
  case AUDIO_S16: {
    const int bytesPerSample = sizeof(int16_t);
    SDL_Log("its a signed 16bit int!");
    int16_t *stream = (int16_t *)wavBuffer;

    SDL_Log("sample info: %d %f %d - %f", wavLength,
            wavLength / float(bytesPerSample), wavSpec.freq,
            float(wavLength / float(bytesPerSample * wavSpec.channels)) /
                float(wavSpec.freq));
    bufferSize = wavLength / bytesPerSample;
    sampleInfo = new (arena->push<SampleBuffer<float>>())
        SampleBuffer<float>(arena, wavSpec.freq, bufferSize, wavSpec.channels);
    if (sampleInfo == NULL) {
      SDL_LogError(0, "sample failed to init!");
    }
    if (sampleInfo->buffer == NULL) {
      return NULL;
    }
    for (size_t i = 0; i < bufferSize; ++i) {
      sampleInfo->buffer[i] = 0;
      for (size_t ch = 0; ch < wavSpec.channels; ++ch) {
        sampleInfo->buffer[i] +=
            float(stream[i + ch]) / float(std::numeric_limits<int16_t>::max());
      }
    }

    break;
  }
  case AUDIO_U16: {
    const int bytesPerSample = sizeof(uint16_t);
    SDL_Log("its a unsigned 16 bit int!");
    break;
  }
  case AUDIO_S32: {
    const int bytesPerSample = sizeof(int32_t);
    SDL_Log("its a signed 32 bit int!");
    int32_t *stream = (int32_t *)wavBuffer;

    SDL_Log("sample info: %d %f %d - %f", wavLength,
            wavLength / float(bytesPerSample), wavSpec.freq,
            float(wavLength / float(bytesPerSample * wavSpec.channels)) /
                float(wavSpec.freq));
    bufferSize = wavLength / bytesPerSample;
    sampleInfo = new (arena->push<SampleBuffer<float>>())
        SampleBuffer<float>(arena, wavSpec.freq, bufferSize, wavSpec.channels);
    if (sampleInfo == NULL) {
      SDL_LogError(0, "sample failed to init!");
    }
    if (sampleInfo->buffer == NULL) {
      return NULL;
    }
    for (size_t i = 0; i < bufferSize; ++i) {
      sampleInfo->buffer[i] = 0;
      for (size_t ch = 0; ch < wavSpec.channels; ++ch) {
        sampleInfo->buffer[i] +=
            float(stream[i + ch]) / float(std::numeric_limits<int32_t>::max());
      }
    }

    break;
  }
  case AUDIO_F32: {
    const int bytesPerSample = sizeof(float);
    SDL_Log("its a 32bit float");
    float *stream = (float *)wavBuffer;

    SDL_Log("sample info: %d %f %d - %f", wavLength,
            wavLength / float(bytesPerSample), wavSpec.freq,
            float(wavLength / float(bytesPerSample * wavSpec.channels)) /
                float(wavSpec.freq));
    bufferSize = wavLength / bytesPerSample;
    sampleInfo = new (arena->push<SampleBuffer<float>>())
        SampleBuffer<float>(arena, wavSpec.freq, bufferSize, wavSpec.channels);
    if (sampleInfo == NULL) {
      SDL_LogError(0, "sample failed to init!");
    }
    if (sampleInfo->buffer == NULL) {
      return NULL;
    }
    for (size_t i = 0; i < bufferSize; ++i) {
      sampleInfo->buffer[i] = 0;
      for (size_t ch = 0; ch < wavSpec.channels; ++ch) {
        sampleInfo->buffer[i] += float(stream[i + ch]);
      }
    }
    SDL_Log("its a float!");
    break;
  }
  default:
    SDL_Log("could not detect datatype");
  }

  free(wavBuffer);

  SDL_Log("loaded sample!");
  return sampleInfo;
}
