#pragma once

#include "collider.h"
#include "metaphor.h"
#include "save_state.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_radio_button.h"
#include "widget_state.h"

struct InstrumentMetaphorSelectionPopupUI {
  enum Mode { OPEN, CLOSED } mode = CLOSED;
  SaveState *saveState = NULL;
  RadioGroup metaphorOptions;
  AxisAlignedBoundingBox shape;
  AxisAlignedBoundingBox modalBackground;
  InstrumentMetaphorSelectionPopupUI(SaveState *_saveState)
      : saveState(_saveState) {}
  void open() {
    for (size_t i = 0; i < NUM_INSTRUMENT_METAPHOR_TYPES; ++i) {
      if (InstrumentMetaphorTypes[i] ==
          saveState->getInstrumentMetaphorType()) {
        metaphorOptions.selectedIndex = i;
        metaphorOptions.options[i].state = ACTIVE;

      } else {
        metaphorOptions.options[i].state = INACTIVE;
      }
    }

    mode = OPEN;
  }
  void close() { mode = CLOSED; }
  bool isOpen() { return mode == OPEN; }
  void buildLayout(AxisAlignedBoundingBox shape) {
    this->shape = shape;
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;

    modalBackground.halfSize = shape.halfSize.scale(2);

    std::vector<std::string> instrumentOptionLabels = {};
    for (auto &instrumentType : InstrumentMetaphorTypes) {
      instrumentOptionLabels.push_back(getDisplayName(instrumentType));
    }

    metaphorOptions = RadioGroup(instrumentOptionLabels,
                                 saveState->getInstrumentMetaphorType());
    auto radioGroupShape = AxisAlignedBoundingBox{
        .position = shape.position,
        .halfSize = {.x = static_cast<float>(shape.halfSize.x * 0.8),
                     .y = static_cast<float>(shape.halfSize.y / 16.0)}};

    metaphorOptions.options[KEYBOARD].iconType = IconType::PIANO;
    metaphorOptions.options[TOUCH_PAD].iconType = IconType::TOUCH;
    metaphorOptions.options[GAME].iconType = IconType::GAME_PAD;
    metaphorOptions.options[SEQUENCER].iconType = IconType::GRID;

    metaphorOptions.buildLayout(radioGroupShape);
  }
  void handleMouseDown(vec2f_t mousePosition) {
    DoClickRadioGroup(&metaphorOptions, mousePosition);
  }

  void handleMouseUp(vec2f_t mousePosition) {
    if (DoClickRadioGroup(&metaphorOptions, mousePosition)) {
      saveState->setInstrumentMetaphor(
          InstrumentMetaphorTypes[metaphorOptions.selectedIndex]);
      mode = CLOSED;
    }
  }
  void draw(SDL_Renderer *renderer, const Style &style) {
    DrawFilledRect(modalBackground, renderer, {0, 0, 0, 128});
    DrawRadioGroup(&metaphorOptions, renderer, style);
  }
};
