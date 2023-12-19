#pragma once

#include "sensor.h"
#include <cstddef>
enum InstrumentMetaphorType {
  KEYBOARD,
  SEQUENCER,
  TOUCH_PAD
  /*  GAME, SHAKER, DRUMHEAD*/
};

static const size_t NUM_INSTRUMENT_METAPHOR_TYPES = 3;
static_assert(TOUCH_PAD == NUM_INSTRUMENT_METAPHOR_TYPES - 1,
              "enum and table size must agree");
static const InstrumentMetaphorType
    InstrumentMetaphorTypes[NUM_INSTRUMENT_METAPHOR_TYPES] = {
        KEYBOARD, SEQUENCER, TOUCH_PAD};

static const char *getDisplayName(const InstrumentMetaphorType type) {
  static const char
      *InstrumentMetaphorTypeDisplayNames[NUM_INSTRUMENT_METAPHOR_TYPES] = {
          "keyboard", "sequencer", "touch pad"};
  return InstrumentMetaphorTypeDisplayNames[static_cast<int>(type)];
}

static const bool
isInstrumentInputType(const InstrumentMetaphorType instrumentType,
                      const ContinuousInputType inputType) {

  switch (instrumentType) {

  case KEYBOARD:
    return inputType == ContinuousInputType::KEYBOARD_KEY;
  case SEQUENCER:
    return inputType == ContinuousInputType::SEQUENCER_STEP_LEVEL;
  case TOUCH_PAD:
    return (inputType == ContinuousInputType::TOUCH_X_POSITION) ||
           (inputType == ContinuousInputType::TOUCH_Y_POSITION);
    break;
  }
  return false;
}
