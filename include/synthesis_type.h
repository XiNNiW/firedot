#pragma once

#include <cstddef>
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
