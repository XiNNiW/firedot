#pragma once

#include <cstddef>
enum InstrumentMetaphorType {
  KEYBOARD,
  SEQUENCER,
  /*  GAME, SHAKER, DRUMHEAD*/
};

static const size_t NUM_INSTRUMENT_METAPHOR_TYPES = 2;
static_assert(SEQUENCER == NUM_INSTRUMENT_METAPHOR_TYPES - 1,
              "enum and table size must agree");
static const InstrumentMetaphorType
    InstrumentMetaphorTypes[NUM_INSTRUMENT_METAPHOR_TYPES] = {KEYBOARD,
                                                              SEQUENCER};
static const char
    *InstrumentMetaphorTypeDisplayNames[NUM_INSTRUMENT_METAPHOR_TYPES] = {
        "keyboard", "sequencer"};
