#pragma once

#include "collider.h"
#include "metaphor.h"
#include "save_state.h"
#include "synthesis.h"
#include "widget_radio_button.h"
struct InstrumentMetaphorSelectorUI {
  AxisAlignedBoundingBox shape;
  RadioGroup metaphorOptions;
  SaveState *saveState = NULL;
  Synthesizer<float> *synth = NULL;
  InstrumentMetaphorSelectorUI(SaveState *_saveState,
                               Synthesizer<float> *_synth)
      : saveState(_saveState), synth(_synth) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    std::vector<std::string> instrumentOptionLabels = {};
    for (auto &instrumentType : InstrumentMetaphorTypes) {
      instrumentOptionLabels.push_back(getDisplayName(instrumentType));
    }

    metaphorOptions = RadioGroup(instrumentOptionLabels,
                                 saveState->getInstrumentMetaphorType());
    auto radioGroupShape = AxisAlignedBoundingBox{
        .position = shape.position,
        .halfSize = {
            .x = static_cast<float>(shape.halfSize.x - shape.halfSize.x / 24.0),
            .y = static_cast<float>(shape.halfSize.y -
                                    shape.halfSize.x / 24.0)}};

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
          InstrumentMetaphorTypes[metaphorOptions.selectedIndex], synth);
    }
  }

  void draw(SDL_Renderer *renderer, const Style &style) {
    DrawRadioGroup(&metaphorOptions, renderer, style);
  }
};
