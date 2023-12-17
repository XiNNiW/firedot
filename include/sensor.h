#pragma once

#include "synthesis.h"
#include "synthesis_parameter.h"
#include <cstddef>
#include <map>

enum ContinuousInputType {
  TILT,
  ACCELERATION,
  SPIN_VELOCITY,
  KEYBOARD_KEY,
  SEQUENCER_STEP_LEVEL,
  TOUCH_X_POSITION,
  TOUCH_Y_POSITION,
};
static const size_t NUM_CONTINUOUS_INPUT_TYPES = 7;
static_assert(TOUCH_Y_POSITION == NUM_CONTINUOUS_INPUT_TYPES - 1,
              "enum and table size must agree");
static const ContinuousInputType
    ContinuousInputTypes[NUM_CONTINUOUS_INPUT_TYPES] = {TILT,
                                                        ACCELERATION,
                                                        SPIN_VELOCITY,
                                                        KEYBOARD_KEY,
                                                        SEQUENCER_STEP_LEVEL,
                                                        TOUCH_X_POSITION,
                                                        TOUCH_Y_POSITION};
static const char *ContinuousInputTypeDisplayNames[NUM_CONTINUOUS_INPUT_TYPES] =
    {"tilt",       "acceleration", "spin velocity", "key",
     "step level", "touch x",      "touch y"};

enum MomentaryInputType { TOUCH, SHAKE, SEQUENCER_GATE };
static const size_t NUM_MOMENTARY_INPUT_TYPES = 3;
static_assert(SEQUENCER_GATE == NUM_MOMENTARY_INPUT_TYPES - 1,
              "enum and table size must agree");
static const MomentaryInputType MomentaryInputTypes[NUM_MOMENTARY_INPUT_TYPES] =
    {TOUCH, SHAKE, SEQUENCER_GATE};
static const char *MomentaryInputTypeDisplayNames[NUM_MOMENTARY_INPUT_TYPES] = {
    "touch", "shake", "sequencer gate"};

template <typename sample_t> struct InputMapping {
  std::map<ContinuousInputType, ContinuousParameterType> continuous_mappings;
  std::map<MomentaryInputType, MomentaryParameterType> momentary_mappings;

  inline void emitEvent(Synthesizer<sample_t> *synth, ContinuousInputType type,
                        sample_t value) {
    for (auto &pair : continuous_mappings) {
      auto sensorType = pair.first;
      auto parameterEventType = pair.second;

      if (type == sensorType) {
        synth->pushParameterChangeEvent(parameterEventType, value);
      }
    }
  }

  inline void emitEvent(Synthesizer<sample_t> *synth, MomentaryInputType type,
                        sample_t value) {
    for (auto &pair : momentary_mappings) {
      auto sensorType = pair.first;
      auto parameterEventType = pair.second;

      if (type == sensorType) {
        synth->pushGateEvent(parameterEventType, value);
      }
    }
  }
  // inline void noteOn(Synthesizer<sample_t> *synth, InputType inputType,
  //                    sample_t noteValue) {
  //   for (auto &pair : mapping) {
  //     auto sensorType = pair.first;
  //     auto parameterEventType = pair.second;
  //     if (inputType == sensorType) {
  //       synth->noteOn(parameterEventType, noteValue);
  //     }
  //   }
  // }

  // inline void noteOff(Synthesizer<sample_t> *synth, InputType inputType,
  //                     sample_t noteValue) {
  //   for (auto &pair : mapping) {
  //     auto sensorType = pair.first;
  //     auto parameterEventType = pair.second;
  //     if (inputType == sensorType) {
  //       synth->noteOff(parameterEventType, noteValue);
  //     }
  //   }
  // }

  inline void addMapping(ContinuousInputType sensorType,
                         ContinuousParameterType paramType) {
    continuous_mappings[sensorType] = paramType;
  }

  inline void removeMapping(ContinuousInputType sensorType,
                            ContinuousParameterType paramType) {
    continuous_mappings.erase(sensorType);
  }

  inline const bool isMapped(const ContinuousParameterType parameterType) {
    for (auto &pair : continuous_mappings) {
      if (pair.second == parameterType)
        return true;
    }
    return false;
  }

  inline void
  removeMappingForParameterType(ContinuousParameterType parameterType) {
    bool shouldRemove = false;
    ContinuousInputType sensorType;
    for (auto &keyValuePair : continuous_mappings) {
      if (keyValuePair.second == parameterType) {
        shouldRemove = true;
        sensorType = keyValuePair.first;
      }
    }
    if (shouldRemove) {
      removeMapping(sensorType, parameterType);
    }
  }
};
