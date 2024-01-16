#pragma once

#include <cstddef>
enum InstrumentMetaphorType {
  KEYBOARD,
  SEQUENCER,
  TOUCH_PAD,
  GAME,
  InstrumentMetaphorType__SIZE
};

static const size_t NUM_INSTRUMENT_METAPHOR_TYPES =
    InstrumentMetaphorType__SIZE;

static const InstrumentMetaphorType
    InstrumentMetaphorTypes[NUM_INSTRUMENT_METAPHOR_TYPES] = {
        KEYBOARD, SEQUENCER, TOUCH_PAD, GAME};

static const char *getDisplayName(const InstrumentMetaphorType type) {
  switch (type) {
  case KEYBOARD:
    return "keyboard";
  case SEQUENCER:
    return "sequencer";
  case TOUCH_PAD:
    return "touch pad";
  case GAME:
    return "game";
  case InstrumentMetaphorType__SIZE:
    return "";
    break;
  }
  return "";
}
