#pragma once

#include "synthesis.h"
#include <cstddef>
#include <map>
enum SensorType { TILT, SPIN };
static const size_t NUM_SENSOR_TYPES = 2;
static_assert(SPIN == NUM_SENSOR_TYPES - 1, "enum and table size must agree");
static const SensorType SensorTypes[NUM_SENSOR_TYPES] = {TILT, SPIN};
static const char *SensorTypesDisplayNames[NUM_SENSOR_TYPES] = {"tilt", "spin"};

template <typename sample_t> struct SensorMapping {
  // std::set<std::pair<SensorType, ParameterType>> mapping;
  std::map<SensorType, ParameterType> mapping;

  inline void emitEvent(Synthesizer<sample_t> *synth, SensorType type,
                        sample_t value) {
    for (auto &pair : mapping) {
      auto sensorType = pair.first;
      auto parameterEventType = pair.second;
      if (type == sensorType) {
        synth->pushParameterChangeEvent(parameterEventType, value);
      }
    }
  }

  inline void addMapping(SensorType sensorType, ParameterType paramType) {
    // mapping.insert(std::pair(sensorType, paramType));
    mapping[sensorType] = paramType;
  }

  inline void removeMapping(SensorType sensorType, ParameterType paramType) {
    // mapping.erase(std::pair(sensorType, paramType));
    mapping.erase(sensorType);
  }
};
