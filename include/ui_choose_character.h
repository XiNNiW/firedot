#pragma once

#include "collider.h"
#include "metaphor.h"
#include "save_state.h"
#include "synthesis_parameter.h"
#include "widget_label.h"
#include "widget_radio_button.h"
#include "widget_state.h"
#include "widget_utils.h"

struct ChooseCharacterUI {
  SaveState *saveState;
  RadioGroup characterChooser;
  std::string pageTitleLabel = "choose your character!";
  AxisAlignedBoundingBox pageTitleLabelShape;
  AxisAlignedBoundingBox bodyShape;

  ChooseCharacterUI(SaveState *_saveState) : saveState(_saveState) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto pageLabelHeight = 50;
    pageTitleLabelShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = static_cast<float>(pageLabelHeight / 2.0)},
        .halfSize = {.x = shape.halfSize.x,
                     .y = static_cast<float>(pageLabelHeight / 2.0)}};
    bodyShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = shape.position.y + pageLabelHeight},
        .halfSize = {
            .x = shape.halfSize.x,
            .y = static_cast<float>(shape.halfSize.y - pageLabelHeight / 2.0)}};
    std::vector<std::string> instrumentOptionLabels = {};
    for (auto &instrumentType : InstrumentMetaphorTypes) {
      instrumentOptionLabels.push_back(getDisplayName(instrumentType));
    }
    characterChooser = RadioGroup::MakeRadioGroup(instrumentOptionLabels, 0);
    characterChooser.buildLayout(
        {.position = {.x = bodyShape.halfSize.x, .y = bodyShape.halfSize.y},
         .halfSize = {.x = bodyShape.halfSize.x - 30, .y = 75}});
  };

  void handleFingerMove(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure){};

  void handleFingerDown(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure){};

  void handleFingerUp(const SDL_FingerID &fingerId, const vec2f_t &position,
                      const float pressure){};

  void handleMouseMove(const vec2f_t &mousePosition){};

  void handleMouseDown(const vec2f_t &mousePosition) {
    int selectedIndex = 0;
    if (DoClickRadioGroup(&characterChooser, mousePosition)) {
      saveState->instrumentMetaphor =
          InstrumentMetaphorTypes[characterChooser.selectedIndex];
    }
  };

  void handleMouseUp(const vec2f_t &mousePosition){};

  void draw(SDL_Renderer *renderer, const Style &style) {
    auto pageLabelRect = ConvertAxisAlignedBoxToSDL_Rect(pageTitleLabelShape);
    DrawLabel(pageTitleLabel, style.hoverColor, style.unavailableColor,
              pageLabelRect, renderer, style, HorizontalAlignment::CENTER,
              VerticalAlignment::CENTER);
    DrawRadioGroup(characterChooser, renderer, style);
  };
};
