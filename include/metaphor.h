#pragma once

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
static const char
    *InstrumentMetaphorTypeDisplayNames[NUM_INSTRUMENT_METAPHOR_TYPES] = {
        "keyboard", "sequencer", "touch pad"};
