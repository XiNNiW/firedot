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

static const size_t NUM_INSTRUMENT_INPUT_TYPES = 4;
static const ContinuousInputType
    InstrumentInputTypes[NUM_INSTRUMENT_INPUT_TYPES] = {
        KEYBOARD_KEY, SEQUENCER_STEP_LEVEL, TOUCH_X_POSITION, TOUCH_Y_POSITION};

static const size_t NUM_SENSOR_INPUT_TYPES = 3;
static const ContinuousInputType SensorInputTypes[NUM_SENSOR_INPUT_TYPES] = {
    TILT, ACCELERATION, SPIN_VELOCITY};
static_assert(NUM_INSTRUMENT_INPUT_TYPES + NUM_SENSOR_INPUT_TYPES ==
                  NUM_CONTINUOUS_INPUT_TYPES,
              "input types are either sensor or instrument");

static const char *getDisplayName(ContinuousInputType type) {
  static const char
      *ContinuousInputTypeDisplayNames[NUM_CONTINUOUS_INPUT_TYPES] = {
          "tilt",       "acceleration", "spin velocity", "key",
          "step level", "touch x",      "touch y"};
  return ContinuousInputTypeDisplayNames[static_cast<int>(type)];
}

enum MomentaryInputType { INSTRUMENT_GATE, SHAKE };
static const size_t NUM_MOMENTARY_INPUT_TYPES = 2;
static_assert(SHAKE == NUM_MOMENTARY_INPUT_TYPES - 1,
              "enum and table size must agree");
static const MomentaryInputType MomentaryInputTypes[NUM_MOMENTARY_INPUT_TYPES] =
    {INSTRUMENT_GATE, SHAKE};

static const char *getDisplayName(MomentaryInputType type) {
  static const char *MomentaryInputTypeDisplayNames[NUM_MOMENTARY_INPUT_TYPES] =
      {"instrument gate", "shake"};
  return MomentaryInputTypeDisplayNames[static_cast<int>(type)];
}

struct NoteMap {
  static constexpr size_t NUM_NOTES = 35;
  float notes[NUM_NOTES] = {
      36,         37,         38,         39,         40,         36 + 5,
      37 + 5,     38 + 5,     39 + 5,     40 + 5,     36 + 2 * 5, 37 + 2 * 5,
      38 + 2 * 5, 39 + 2 * 5, 40 + 2 * 5, 36 + 3 * 5, 37 + 3 * 5, 38 + 3 * 5,
      39 + 3 * 5, 40 + 3 * 5, 36 + 4 * 5, 37 + 4 * 5, 38 + 4 * 5, 39 + 4 * 5,
      40 + 4 * 5, 36 + 5 * 5, 37 + 5 * 5, 38 + 5 * 5, 39 + 5 * 5, 40 + 5 * 5,
      36 + 6 * 5, 37 + 6 * 5, 38 + 6 * 5, 39 + 6 * 5, 40 + 6 * 5,
  };
  inline float getFrequency(const float value) {
    return notes[static_cast<int>(floor(clip(value) * (NUM_NOTES - 1)))];
  }
};

template <typename sample_t> struct InputMapping {
  std::map<ContinuousInputType, ContinuousParameterType> continuous_mappings;
  std::map<MomentaryInputType, MomentaryParameterType> momentary_mappings;
  NoteMap noteMap;
  inline void emitEvent(Synthesizer<sample_t> *synth, ContinuousInputType type,
                        sample_t value) {

    for (auto &pair : continuous_mappings) {
      auto sensorType = pair.first;
      auto parameterEventType = pair.second;

      if (type == sensorType) {
        switch (parameterEventType) {
        case FREQUENCY:
          value = mtof(noteMap.getFrequency(value));
          break;
        default:
          break;
        }
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

  inline void addMapping(ContinuousInputType sensorType,
                         ContinuousParameterType paramType) {
    continuous_mappings[sensorType] = paramType;
  }

  inline void removeMapping(ContinuousInputType sensorType,
                            ContinuousParameterType paramType) {
    continuous_mappings.erase(sensorType);
  }

  inline void addMapping(MomentaryInputType sensorType,
                         MomentaryParameterType paramType) {
    momentary_mappings[sensorType] = paramType;
  }

  inline void removeMapping(MomentaryInputType sensorType,
                            MomentaryParameterType paramType) {
    momentary_mappings.erase(sensorType);
  }

  inline const bool isMapped(const ContinuousParameterType parameterType) {
    for (auto &pair : continuous_mappings) {
      if (pair.second == parameterType)
        return true;
    }
    return false;
  }

  inline const bool isMapped(const MomentaryParameterType parameterType) {
    for (auto &pair : momentary_mappings) {
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

  inline void
  removeMappingForParameterType(MomentaryParameterType parameterType) {
    bool shouldRemove = false;
    MomentaryInputType sensorType;
    for (auto &keyValuePair : momentary_mappings) {
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
