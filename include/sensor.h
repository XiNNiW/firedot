#pragma once

#include "synthesis.h"
#include <cstddef>
#include <map>

enum SensorType { TILT, ROLL, PITCH, YAW, ACCELERATION, ACC_X, ACC_Y, ACC_Z };
static const size_t NUM_SENSOR_TYPES = 8;
static_assert(ACC_Z == NUM_SENSOR_TYPES - 1, "enum and table size must agree");
static const SensorType SensorTypes[NUM_SENSOR_TYPES] = {
    TILT, ROLL, PITCH, YAW, ACCELERATION, ACC_X, ACC_Y, ACC_Z};
static const char *SensorTypesDisplayNames[NUM_SENSOR_TYPES] = {
    "tilt",         "roll",           "pitch",          "yaw",
    "acceleration", "acceleration x", "acceleration y", "acceleration z"};

template <typename sample_t> struct SensorMapping {
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

  inline const bool isMapped(const ParameterType parameterType) {
    for (auto &pair : mapping) {
      if (pair.second == parameterType)
        return true;
    }
    return false;
  }
};
