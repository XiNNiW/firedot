
#pragma once
#include "SDL_audio.h"
#include "SDL_log.h"
#include "synthesis_abstract.h"
#include "synthesis_parameter.h"
#include <algae.h>
#include <cstddef>
#include <cstdlib>
#include <vector>

using algae::dsp::control::ASREnvelope;
using algae::dsp::filter::Biquad;
using algae::dsp::math::clamp;
using algae::dsp::math::clip;
using algae::dsp::math::lerp;
using algae::dsp::math::mtof;
using algae::dsp::oscillator::blep;
using algae::dsp::oscillator::computePhaseIncrement;

class Arena {
private:
  char *data;
  size_t position;
  size_t size;

public:
  Arena(size_t _size) : size(_size), position(0), data((char *)malloc(_size)) {}
  template <typename T> bool canAlloc() const {
    return (position + sizeof(T)) < size;
  }
  template <typename T> bool canAllocArray(size_t length) const {
    return (position + sizeof(T) * length) < size;
  }
  template <typename T> T *push() {
    size_t size = sizeof(T);
    void *ptr = data + size;
    position += size;
    return (T *)ptr;
  }
  template <typename T> T *pushArray(size_t length) {
    size_t size = sizeof(T) * length;
    void *ptr = data + size;
    position += size;
    return (T *)ptr;
  }
  template <typename T> void pop() { position -= sizeof(T); }
  inline void clear() { position = 0; }
  inline size_t getPosition() { return position; }
  ~Arena() { free(data); }
};

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

static inline SampleBuffer<float> *LoadWAVSampleAsMono(Arena *arena,
                                                       std::string samplePath) {

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

template <typename sample_t> struct SampleBank {
  Arena arena;
  SampleBuffer<sample_t> *buffers[16];
  size_t numSamples = 0;
  size_t size;
  SampleBank(size_t totalSizeInSamples)
      : arena(Arena(totalSizeInSamples * sizeof(sample_t))) {}
  void clear() {
    buffers.clear();
    arena.clear();
  }
  bool loadSample(const std::string &path) {
    if (numSamples == 16)
      return false;
    auto sampleBuffer = LoadWAVSampleAsMono(&arena, path);
    if (sampleBuffer != NULL) {
      buffers[numSamples++] = sampleBuffer;
      return true;
    } else {
      return false;
    }
  }
};

struct AudioSample {
  float sampleRate;
  size_t numChannels;
  size_t size;
  float *buffer;
  AudioSample(const float _sampleRate, const size_t _numChannels,
              const size_t _bufferSize)
      : sampleRate(_sampleRate), numChannels(_numChannels), size(_bufferSize),
        buffer((float *)calloc(size, sizeof(float))) {}
  ~AudioSample() {
    if (buffer) {
      free(buffer);
    }
  }
};

inline void FreeSampleInfo(AudioSample *sampleInfo) {
  // free(sampleInfo->buffer);
  // free(sampleInfo);
  delete sampleInfo;
}

static inline bool LoadWAVSampleAsMono(std::string samplePath,
                                       AudioSample **sampleInfo) {

  SDL_AudioSpec wavSpec;
  Uint32 wavLength;
  Uint8 *wavBuffer;
  long bufferSize = 0;

  SDL_LoadWAV(samplePath.c_str(), &wavSpec, &wavBuffer, &wavLength);

  bool isSupportedFormat = false;

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
    isSupportedFormat = true;
    SDL_Log("its a signed 16bit int!");
    int16_t *stream = (int16_t *)wavBuffer;

    SDL_Log("sample info: %d %f %d - %f", wavLength,
            wavLength / float(bytesPerSample), wavSpec.freq,
            float(wavLength / float(bytesPerSample * wavSpec.channels)) /
                float(wavSpec.freq));
    bufferSize = wavLength / bytesPerSample;
    *sampleInfo = new AudioSample(wavSpec.freq, wavSpec.channels, bufferSize);
    if (*sampleInfo == NULL) {
      SDL_LogError(0, "sample failed to init!");
    }
    if ((*sampleInfo)->buffer == NULL) {
      return false;
    }
    for (size_t i = 0; i < bufferSize; ++i) {
      (*sampleInfo)->buffer[i] = 0;
      for (size_t ch = 0; ch < wavSpec.channels; ++ch) {
        (*sampleInfo)->buffer[i] +=
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
    break;
  }
  case AUDIO_F32: {
    const int bytesPerSample = sizeof(float);
    isSupportedFormat = true;
    SDL_Log("its a float!");
    break;
  }
  default:
    SDL_Log("could not detect datatype");
  }

  free(wavBuffer);
  if ((*sampleInfo) == NULL) {

    SDL_LogError(0, "freeing wav buffer killed sample info!");
  }
  SDL_Log("loaded sample!");
  return isSupportedFormat;
}

template <typename sample_t> struct SamplerVoice {
  sample_t frequency;
  ASREnvelope<sample_t> env;
  Biquad<sample_t, sample_t> filter;
  // sample_t *buffer = NULL;
  // size_t bufferSize = 0;
  SampleBank<sample_t> *sampleBank = NULL;
  sample_t sampleRate = 48000;
  sample_t originalSampleRate = 48000;
  sample_t gain = 0;
  sample_t phases[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  sample_t phaseIncrements[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0};
  sample_t soundSource = 0;
  sample_t attackTime = 10;
  sample_t releaseTime = 1000;
  sample_t active = 0;
  sample_t filterCutoff = 10000;
  sample_t filterQuality = 0.01;

  SamplerVoice<sample_t>(SampleBank<sample_t> *bank) : sampleBank(bank) {
    init();
  }

  inline void init() {
    filter.lowpass(filterCutoff, filterQuality, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);
  }

  inline void setSampleRate(sample_t sampleRate) {
    this->sampleRate = sampleRate;
    init();
  }

  inline void setFrequency(const sample_t frequency,
                           const sample_t sampleRate) {

    for (size_t i = 0; i < sampleBank->numSamples; i++) {
      auto bufferSize = sampleBank->buffers[i]->bufferSize;
      sample_t ratio = frequency / mtof<sample_t>(36);
      phaseIncrements[i] = ratio * (1.0 / sample_t(bufferSize)) *
                           (sampleRate / originalSampleRate);
    }
  }

  inline void setGate(sample_t gate) { env.setGate(gate); }

  inline const sample_t next() {

    auto nextFrequency = frequency;
    setFrequency(nextFrequency, sampleRate);
    env.set(attackTime, releaseTime, sampleRate);

    auto envelopeSample = env.next();
    if (env.stage == ASREnvelope<sample_t>::Stage::OFF) {
      active = false;
    }

    filter.lowpass(filterCutoff, filterQuality, sampleRate);

    sample_t sampleIndex =
        soundSource * static_cast<sample_t>(sampleBank->numSamples - 1);
    int s1 = floor(sampleIndex);
    int s2 = (s1 + 1) % sampleBank->numSamples;
    sample_t mantissa = sampleIndex - static_cast<sample_t>(s1);

    auto out = lerp(sampleBank->buffers[s1]->read(phases[s1]),
                    sampleBank->buffers[s2]->read(phases[s2]), mantissa);

    out = filter.next(out);
    out *= env.next() * 4;
    for (size_t i = 0; i < sampleBank->numSamples; ++i) {
      phases[i] += phaseIncrements[i];
      phases[i] = phases[i] > 1 ? phases[i] - 1 : phases[i];
    }

    return out;
  }
};

template <typename sample_t>
struct Sampler : AbstractMonophonicSynthesizer<sample_t, Sampler<sample_t>> {
  sample_t *buffer;
  size_t bufferSize;
  SamplerVoice<sample_t> voice;

  inline void setSampleRate(sample_t sr) {
    this->sampleRate = sr;
    voice.setSampleRate(this->sampleRate);
  }

  Sampler<sample_t>(SampleBank<sample_t> *bank) : voice(bank) {
    setSampleRate(this->sampleRate);
  }
};
