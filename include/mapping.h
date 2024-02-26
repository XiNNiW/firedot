#pragma once

#include "metaphor.h"
#include "pitch_collection.h"
#include "synthesis.h"
#include "synthesis_parameter.h"
#include <cstddef>
#include <map>
#include <vector>

enum ContinuousInputType {
  TILT,
  ACCELERATION,
  SPIN_VELOCITY,
  KEYBOARD_KEY,
  SEQUENCER_STEP_LEVEL,
  TOUCH_X_POSITION,
  TOUCH_Y_POSITION,
  COLLISION_VELOCITY,
  COLLISION_POSITION_X,
  COLLISION_POSITION_Y,
  ContinuousInputType_SIZE
};
static const size_t NUM_CONTINUOUS_INPUT_TYPES = ContinuousInputType_SIZE;

static const ContinuousInputType
    ContinuousInputTypes[NUM_CONTINUOUS_INPUT_TYPES] = {
        TILT,
        ACCELERATION,
        SPIN_VELOCITY,
        KEYBOARD_KEY,
        SEQUENCER_STEP_LEVEL,
        TOUCH_X_POSITION,
        TOUCH_Y_POSITION,
        COLLISION_VELOCITY,
        COLLISION_POSITION_X,
        COLLISION_POSITION_Y,
};

static const size_t NUM_INSTRUMENT_INPUT_TYPES = 7;
static const ContinuousInputType
    InstrumentInputTypes[NUM_INSTRUMENT_INPUT_TYPES] = {
        KEYBOARD_KEY,        SEQUENCER_STEP_LEVEL, TOUCH_X_POSITION,
        TOUCH_Y_POSITION,    COLLISION_VELOCITY,   COLLISION_POSITION_X,
        COLLISION_POSITION_Y};

static const size_t NUM_SENSOR_INPUT_TYPES = 3;
static const ContinuousInputType SensorInputTypes[NUM_SENSOR_INPUT_TYPES] = {
    TILT, ACCELERATION, SPIN_VELOCITY};
static_assert(NUM_INSTRUMENT_INPUT_TYPES + NUM_SENSOR_INPUT_TYPES ==
                  NUM_CONTINUOUS_INPUT_TYPES,
              "input types are either sensor or instrument");

static const char *getDisplayName(ContinuousInputType type) {
  switch (type) {
  case TILT:
    return "tilt";
  case ACCELERATION:
    return "acceleration";
  case SPIN_VELOCITY:
    return "spin velocity";
  case KEYBOARD_KEY:
    return "keyboard key";
  case SEQUENCER_STEP_LEVEL:
    return "step level";
  case TOUCH_X_POSITION:
    return "touch x";
  case TOUCH_Y_POSITION:
    return "touch y";
  case COLLISION_VELOCITY:
    return "collision velocity";
  case COLLISION_POSITION_X:
    return "collision pos x";
  case COLLISION_POSITION_Y:
    return "collision pos y";
  case ContinuousInputType_SIZE:
    return "";
    break;
  }
  return "";
}

enum MomentaryInputType {
  SHAKE,
  KEYBOARD_GATE,
  COLLISION,
  SEQUENCER_GATE,
  TOUCH_PAD_GATE,
  MomentaryInputType_SIZE
};
static const size_t NUM_MOMENTARY_INPUT_TYPES = MomentaryInputType_SIZE;

static const MomentaryInputType MomentaryInputTypes[NUM_MOMENTARY_INPUT_TYPES] =
    {SHAKE, KEYBOARD_GATE, COLLISION, SEQUENCER_GATE};

static const size_t NUM_MOMENTARY_SENSOR_INPUT_TYPES = 1;
static const MomentaryInputType
    SensorMomentaryInputTypes[NUM_MOMENTARY_SENSOR_INPUT_TYPES] = {SHAKE};
static const size_t NUM_MOMENTARY_INSTRUMENT_INPUT_TYPES = 4;
static const MomentaryInputType
    InstrumentMomentaryInputTypes[NUM_MOMENTARY_INSTRUMENT_INPUT_TYPES] = {
        KEYBOARD_GATE,
        COLLISION,
        SEQUENCER_GATE,
        TOUCH_PAD_GATE,
};
static_assert(NUM_MOMENTARY_SENSOR_INPUT_TYPES +
                      NUM_MOMENTARY_INSTRUMENT_INPUT_TYPES ==
                  NUM_MOMENTARY_INPUT_TYPES,
              "input types are either sensor or instrument");

static const char *getDisplayName(MomentaryInputType type) {
  switch (type) {
  case SHAKE:
    return "shake";
  case KEYBOARD_GATE:
    return "keyboard gate";
  case COLLISION:
    return "collision gate";
  case SEQUENCER_GATE:
    return "sequencer gate";
  case TOUCH_PAD_GATE:
    return "touch pad gate";
    break;
  case MomentaryInputType_SIZE:
    return "";
    break;
  }
  return "";
}
static const size_t NUM_KEYBOARD_MOMENTARY_INPUTS = 1;
static const MomentaryInputType
    KeyboardMomentaryInputs[NUM_KEYBOARD_MOMENTARY_INPUTS] = {
        KEYBOARD_GATE,
};

static const size_t NUM_SEQUENCER_MOMENTARY_INPUTS = 1;
static const MomentaryInputType
    SequencerMomentaryInputs[NUM_SEQUENCER_MOMENTARY_INPUTS] = {
        SEQUENCER_GATE,
};

static const size_t NUM_TOUCHPAD_MOMENTARY_INPUTS = 1;
static const MomentaryInputType
    TouchPadMomentaryInputs[NUM_TOUCHPAD_MOMENTARY_INPUTS] = {
        TOUCH_PAD_GATE,
};

static const size_t NUM_GAME_MOMENTARY_INPUTS = 1;
static const MomentaryInputType GameMomentaryInputs[NUM_GAME_MOMENTARY_INPUTS] =
    {
        COLLISION,
};

inline void
getMomentaryInputsForInstrumentType(InstrumentMetaphorType instrumentType,
                                    std::vector<MomentaryInputType> *list) {
  list->clear();
  switch (instrumentType) {
  case KEYBOARD:
    for (auto &type : KeyboardMomentaryInputs) {
      list->push_back(type);
    }
    break;
  case SEQUENCER:
    for (auto &type : SequencerMomentaryInputs) {
      list->push_back(type);
    }
    break;
  case TOUCH_PAD:
    for (auto &type : TouchPadMomentaryInputs) {
      list->push_back(type);
    }
    break;
  case GAME:
    for (auto &type : GameMomentaryInputs) {
      list->push_back(type);
    }
    break;
  case InstrumentMetaphorType__SIZE:
    break;
  }
}
static const size_t NUM_KEYBOARD_CONTINUOUS_INPUTS = 1;
static const ContinuousInputType
    KeyboardContinuousInputs[NUM_KEYBOARD_CONTINUOUS_INPUTS] = {
        KEYBOARD_KEY,
};

static const size_t NUM_SEQUENCER_CONTINUOUS_INPUTS = 1;
static const ContinuousInputType
    SequencerContinuousInputs[NUM_SEQUENCER_CONTINUOUS_INPUTS] = {
        SEQUENCER_STEP_LEVEL};

static const size_t NUM_TOUCHPAD_CONTINUOUS_INPUTS = 2;
static const ContinuousInputType
    TouchPadContinuousInputs[NUM_TOUCHPAD_CONTINUOUS_INPUTS] = {
        TOUCH_X_POSITION, TOUCH_Y_POSITION};

static const size_t NUM_GAME_CONTINUOUS_INPUTS = 3;
static const ContinuousInputType
    GameContinuousInputs[NUM_GAME_CONTINUOUS_INPUTS] = {
        COLLISION_VELOCITY, COLLISION_POSITION_X, COLLISION_POSITION_Y};
inline void
getContinuousInputsForInstrumentType(InstrumentMetaphorType instrumentType,
                                     std::vector<ContinuousInputType> *list) {
  // list->clear();
  switch (instrumentType) {
  case KEYBOARD:
    for (auto &type : KeyboardContinuousInputs) {
      list->push_back(type);
    }
    break;
  case SEQUENCER:
    for (auto &type : SequencerContinuousInputs) {
      list->push_back(type);
    }
    break;
  case TOUCH_PAD:
    for (auto &type : TouchPadContinuousInputs) {
      list->push_back(type);
    }
    break;
  case GAME:
    for (auto &type : GameContinuousInputs) {
      list->push_back(type);
    }
    break;
  case InstrumentMetaphorType__SIZE:
    break;
  }
}
// struct InputType {
//   union uInputType {
//     ContinuousInputType continuousInputType;
//     MomentaryInputType momentaryInputType;
//     uInputType(ContinuousInputType type) : continuousInputType(type) {}
//     uInputType(MomentaryInputType type) : momentaryInputType(type) {}
//   } object;
//   enum Type { CONTINUOUS, MOMENTARY } type;
//   InputType(ContinuousInputType continuousInputType)
//       : object(continuousInputType), type(CONTINUOUS) {}
//   InputType(MomentaryInputType momentaryInputType)
//       : object(momentaryInputType), type(MOMENTARY) {}
// };
//  make this a map of parameter type to input type
template <typename sample_t> struct InputMapping {
  std::map<ContinuousParameterType, ContinuousInputType> continuousMappings =
      std::map<ContinuousParameterType, ContinuousInputType>();
  std::map<MomentaryParameterType, MomentaryInputType> momentaryMappings =
      std::map<MomentaryParameterType, MomentaryInputType>();
  int key = 0;
  ScaleType scaleType = ScaleType::LYDIAN_PENT;
  inline void emitEvent(Synthesizer<sample_t> *synth, ContinuousInputType type,
                        sample_t value) {

    for (auto &pair : continuousMappings) {
      auto sensorType = pair.second;
      auto parameterEventType = pair.first;

      if (type == sensorType) {
        if (parameterEventType == FREQUENCY) {
          value = mtof(ForceToScale(value * 24.0 + 36 + key,
                                    Scales[static_cast<size_t>(scaleType)]));
        }

        synth->pushParameterChangeEvent(parameterEventType, value);
      }
    }
  }

  inline void emitSteppedEvent(Synthesizer<sample_t> *synth,
                               ContinuousInputType type, sample_t value,
                               sample_t numSteps) {

    for (auto &pair : continuousMappings) {
      auto sensorType = pair.second;
      auto parameterEventType = pair.first;

      if (type == sensorType) {
        if (parameterEventType == FREQUENCY) {
          value = mtof(ForceToScale(value + key,
                                    Scales[static_cast<size_t>(scaleType)]));
        } else {
          value /= numSteps;
        }

        synth->pushParameterChangeEvent(parameterEventType, value);
      }
    }
  }

  inline void emitEvent(Synthesizer<sample_t> *synth, MomentaryInputType type,
                        sample_t value) {
    for (auto &pair : momentaryMappings) {
      auto sensorType = pair.second;
      auto parameterEventType = pair.first;

      if (type == sensorType) {
        synth->pushGateEvent(parameterEventType, value);
      }
    }
  }

  inline void addMapping(ContinuousInputType sensorType,
                         ContinuousParameterType paramType) {
    continuousMappings[paramType] = sensorType;
  }

  inline void removeMapping(ContinuousInputType sensorType,
                            ContinuousParameterType paramType) {
    continuousMappings.erase(paramType);
  }

  inline void addMapping(MomentaryInputType sensorType,
                         MomentaryParameterType paramType) {
    momentaryMappings[paramType] = sensorType;
  }

  inline void removeMapping(MomentaryInputType sensorType,
                            MomentaryParameterType paramType) {
    momentaryMappings.erase(paramType);
  }

  inline const bool isMapped(const ContinuousParameterType parameterType) {
    for (auto &pair : continuousMappings) {
      if (pair.first == parameterType)
        return true;
    }
    return false;
  }

  inline const bool isMapped(const MomentaryParameterType parameterType) {
    for (auto &pair : momentaryMappings) {
      if (pair.first == parameterType)
        return true;
    }
    return false;
  }

  inline const bool getMapping(const ContinuousParameterType parameterType,
                               ContinuousInputType *mappedType) {
    for (auto &pair : continuousMappings) {
      if (pair.first == parameterType) {
        *mappedType = pair.second;
        return true;
      }
    }
    return false;
  }

  inline const bool getMapping(const MomentaryParameterType parameterType,
                               MomentaryInputType *mappedType) {
    for (auto &pair : momentaryMappings) {
      if (pair.first == parameterType) {
        *mappedType = pair.second;
        return true;
      }
    }
    return false;
  }

  inline void
  removeMappingForParameterType(ContinuousParameterType parameterType) {
    bool shouldRemove = false;
    ContinuousInputType sensorType;
    for (auto &keyValuePair : continuousMappings) {
      if (keyValuePair.first == parameterType) {
        shouldRemove = true;
        sensorType = keyValuePair.second;
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
    for (auto &keyValuePair : momentaryMappings) {
      if (keyValuePair.first == parameterType) {
        shouldRemove = true;
        sensorType = keyValuePair.second;
      }
    }
    if (shouldRemove) {
      removeMapping(sensorType, parameterType);
    }
  }

  inline void removeMappingForInputType(ContinuousInputType sensorType) {
    bool shouldRemove = false;
    ContinuousParameterType parameterType;
    for (auto &keyValuePair : continuousMappings) {
      if (keyValuePair.second == sensorType) {
        shouldRemove = true;
        parameterType = keyValuePair.first;
      }
    }
    if (shouldRemove) {
      removeMapping(sensorType, parameterType);
    }
  }

  inline void removeMappingForInputType(MomentaryInputType sensorType) {
    bool shouldRemove = false;
    MomentaryParameterType parameterType;
    for (auto &keyValuePair : momentaryMappings) {
      if (keyValuePair.second == sensorType) {
        shouldRemove = true;
        parameterType = keyValuePair.first;
      }
    }
    if (shouldRemove) {
      removeMapping(sensorType, parameterType);
    }
  }
};
