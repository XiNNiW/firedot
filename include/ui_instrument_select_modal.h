#pragma once

#include "collider.h"
#include "save_state.h"
#include "widget_button.h"
#include "widget_radio_button.h"

struct InstrumentSelectModal {
  enum Mode { OPEN, CLOSED, SELECTED } mode;
  RadioGroup instrumentSelectRadioGroup;
  SaveState *saveState = NULL;
  AxisAlignedBoundingBox shape;
  InstrumentSelectModal(SaveState *_saveState) : saveState(_saveState) {}
  inline void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto margin = shape.halfSize.x / 24;
    auto buttonHeight = shape.halfSize.y / 8.0;
    instrumentSelectRadioGroup.buildLayout(
        {.position = shape.position,
         .halfSize = {.x = shape.halfSize.x - margin,
                      .y = static_cast<float>(buttonHeight)}});
  }
  inline void draw(SDL_Renderer *renderer, const Style &style) {
    DrawFilledRect(shape, renderer, SDL_Color{0, 0, 0, 0});
    DrawBoxOutline(shape, renderer, style.color0);
    DrawRadioGroup(&instrumentSelectRadioGroup, renderer, style);
  }
};
