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
  PARTICLE_SIZE,
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
        PARTICLE_SIZE,
};

static const size_t NUM_INSTRUMENT_INPUT_TYPES = 8;
static const ContinuousInputType
    InstrumentInputTypes[NUM_INSTRUMENT_INPUT_TYPES] = {
        KEYBOARD_KEY,         SEQUENCER_STEP_LEVEL, TOUCH_X_POSITION,
        TOUCH_Y_POSITION,     COLLISION_VELOCITY,   COLLISION_POSITION_X,
        COLLISION_POSITION_Y, PARTICLE_SIZE};

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
  case PARTICLE_SIZE:
    return "particle size";
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

static const size_t NUM_GAME_CONTINUOUS_INPUTS = 4;
static const ContinuousInputType
    GameContinuousInputs[NUM_GAME_CONTINUOUS_INPUTS] = {
        COLLISION_VELOCITY, COLLISION_POSITION_X, COLLISION_POSITION_Y,
        PARTICLE_SIZE};
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

struct ModeSpecificMapping {
  std::map<ContinuousParameterType, ContinuousInputType> continuousMappings =
      std::map<ContinuousParameterType, ContinuousInputType>();
  std::map<MomentaryParameterType, MomentaryInputType> momentaryMappings =
      std::map<MomentaryParameterType, MomentaryInputType>();
};

template <typename sample_t> struct InputMapping {

  std::map<InstrumentMetaphorType, ModeSpecificMapping>
      instrumentModeSpecificMappings;

  InputMapping<sample_t>() {
    for (auto &mode : InstrumentMetaphorTypes) {
      instrumentModeSpecificMappings[mode] = ModeSpecificMapping();
      switch (mode) {
      case KEYBOARD:
        addMapping(mode, KEYBOARD_GATE, GATE);
        addMapping(mode, KEYBOARD_KEY, FREQUENCY);
        break;
      case SEQUENCER:
        addMapping(mode, SEQUENCER_GATE, GATE);
        addMapping(mode, SEQUENCER_STEP_LEVEL, FREQUENCY);
        break;
      case TOUCH_PAD:
        addMapping(mode, TOUCH_PAD_GATE, GATE);
        addMapping(mode, TOUCH_Y_POSITION, FREQUENCY);
        addMapping(mode, TOUCH_X_POSITION, SOUND_SOURCE);
        break;
      case GAME:
        addMapping(mode, COLLISION, GATE);
        addMapping(mode, COLLISION_VELOCITY, GAIN);
        break;
      case InstrumentMetaphorType__SIZE:
        break;
      }
    }
  }
  int key = 0;
  ScaleType scaleType = ScaleType::IONIAN_PENT;
  inline void emitEvent(Synthesizer<sample_t> *synth,
                        InstrumentMetaphorType instrumentMode,
                        ContinuousInputType type, sample_t value) {

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    for (auto &pair : continuousMappings) {
      auto sensorType = pair.second;
      auto parameterEventType = pair.first;
      auto mappedValue = value;
      if (type == sensorType) {
        if (parameterEventType == FREQUENCY) {
          mappedValue =
              mtof(key + 36 + ForceToScale(value * 24.0, GetScale(scaleType)));
        }

        synth->pushParameterChangeEvent(parameterEventType, mappedValue);
      }
    }
  }

  inline void emitSteppedEvent(Synthesizer<sample_t> *synth,
                               InstrumentMetaphorType instrumentMode,
                               ContinuousInputType type, sample_t value,
                               sample_t numSteps) {

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    for (auto &pair : continuousMappings) {
      auto sensorType = pair.second;
      auto parameterEventType = pair.first;
      auto mappedValue = value;
      if (type == sensorType) {
        if (parameterEventType == FREQUENCY) {
          mappedValue =
              mtof(key + 36 + ForceToScale(value, GetScale(scaleType)));
        } else {
          mappedValue /= numSteps;
        }

        synth->pushParameterChangeEvent(parameterEventType, mappedValue);
      }
    }
  }

  inline void emitEvent(Synthesizer<sample_t> *synth,
                        InstrumentMetaphorType instrumentMode,
                        MomentaryInputType type, sample_t value) {

    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    for (auto &pair : momentaryMappings) {
      auto sensorType = pair.second;
      auto parameterEventType = pair.first;

      if (type == sensorType) {
        synth->pushGateEvent(parameterEventType, value);
      }
    }
  }

  inline void addMapping(InstrumentMetaphorType instrumentMode,
                         ContinuousInputType sensorType,

                         ContinuousParameterType paramType) {

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    continuousMappings[paramType] = sensorType;
  }

  inline void removeMapping(InstrumentMetaphorType instrumentMode,
                            ContinuousInputType sensorType,

                            ContinuousParameterType paramType) {

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    continuousMappings.erase(paramType);
  }

  inline void addMapping(InstrumentMetaphorType instrumentMode,
                         MomentaryInputType sensorType,
                         MomentaryParameterType paramType) {

    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    momentaryMappings[paramType] = sensorType;
  }

  inline void removeMapping(InstrumentMetaphorType instrumentMode,
                            MomentaryInputType sensorType,
                            MomentaryParameterType paramType) {

    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    momentaryMappings.erase(paramType);
  }

  inline const bool isMapped(InstrumentMetaphorType instrumentMode,
                             ContinuousParameterType parameterType) {

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    for (auto &pair : continuousMappings) {
      if (pair.first == parameterType)
        return true;
    }
    return false;
  }

  inline const bool isMapped(InstrumentMetaphorType instrumentMode,
                             const MomentaryParameterType parameterType) {

    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    for (auto &pair : momentaryMappings) {
      if (pair.first == parameterType)
        return true;
    }
    return false;
  }

  inline const bool getMapping(InstrumentMetaphorType instrumentMode,
                               const ContinuousParameterType parameterType,
                               ContinuousInputType *mappedType) {

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    for (auto &pair : continuousMappings) {
      if (pair.first == parameterType) {
        *mappedType = pair.second;
        return true;
      }
    }
    return false;
  }

  inline const bool getMapping(InstrumentMetaphorType instrumentMode,
                               const MomentaryParameterType parameterType,
                               MomentaryInputType *mappedType) {

    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    for (auto &pair : momentaryMappings) {
      if (pair.first == parameterType) {
        *mappedType = pair.second;
        return true;
      }
    }
    return false;
  }

  inline void
  removeMappingForParameterType(InstrumentMetaphorType instrumentMode,
                                ContinuousParameterType parameterType) {
    bool shouldRemove = false;
    ContinuousInputType sensorType;

    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    for (auto &keyValuePair : continuousMappings) {
      if (keyValuePair.first == parameterType) {
        shouldRemove = true;
        sensorType = keyValuePair.second;
      }
    }
    if (shouldRemove) {
      removeMapping(instrumentMode, sensorType, parameterType);
    }
  }

  inline void
  removeMappingForParameterType(InstrumentMetaphorType instrumentMode,
                                MomentaryParameterType parameterType) {

    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    bool shouldRemove = false;
    MomentaryInputType sensorType;
    for (auto &keyValuePair : momentaryMappings) {
      if (keyValuePair.first == parameterType) {
        shouldRemove = true;
        sensorType = keyValuePair.second;
      }
    }
    if (shouldRemove) {
      removeMapping(instrumentMode, sensorType, parameterType);
    }
  }

  inline void removeMappingForInputType(InstrumentMetaphorType instrumentMode,
                                        ContinuousInputType sensorType) {
    auto &continuousMappings =
        instrumentModeSpecificMappings[instrumentMode].continuousMappings;
    bool shouldRemove = false;
    ContinuousParameterType parameterType;
    for (auto &keyValuePair : continuousMappings) {
      if (keyValuePair.second == sensorType) {
        shouldRemove = true;
        parameterType = keyValuePair.first;
      }
    }
    if (shouldRemove) {
      removeMapping(instrumentMode, sensorType, parameterType);
    }
  }

  inline void removeMappingForInputType(InstrumentMetaphorType instrumentMode,
                                        MomentaryInputType sensorType) {
    auto &momentaryMappings =
        instrumentModeSpecificMappings[instrumentMode].momentaryMappings;
    bool shouldRemove = false;
    MomentaryParameterType parameterType;
    for (auto &keyValuePair : momentaryMappings) {
      if (keyValuePair.second == sensorType) {
        shouldRemove = true;
        parameterType = keyValuePair.first;
      }
    }
    if (shouldRemove) {
      removeMapping(instrumentMode, sensorType, parameterType);
    }
  }
};
