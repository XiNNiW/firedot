#pragma once

#include "SDL_render.h"
#include "collider.h"
#include "sensor.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_radio_button.h"
#include "widget_style.h"
struct MappingSelectPopupUI {

  enum Mode { OPEN, CLOSED, SELECTED } mode = CLOSED;
  std::vector<Button> options;
  AxisAlignedBoundingBox shape;
  int selection = 0;
  Button selectButton;
  Button cancelButton;
  Label title = Label("choose a mapping");
  // InputMapping<float> *mapping;

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;
    auto margin = shape.halfSize.y / 32;

    for (auto &button : options) {
      button.shape = {};
    }

    title.shape = {
        .position = {.x = shape.position.x, .y = yOffset + height / 50},
        .halfSize = {.x = shape.halfSize.x, .y = height / 50}};
    auto buttonMargin = shape.halfSize.x / 24;
    auto buttonHalfWidth = shape.halfSize.x / 2 - buttonMargin;
    auto buttonHalfHeight = shape.halfSize.y / 8 - buttonMargin;
    cancelButton =
        Button{.label = Label("cancel"),
               .shape = {.position = {.x = xOffset + shape.halfSize.x -
                                           buttonHalfWidth - buttonMargin,
                                      .y = yOffset + height - buttonHalfHeight -
                                           buttonMargin},
                         .halfSize = {buttonHalfWidth, buttonHalfHeight}}};
    selectButton =
        Button{.label = Label("select"),
               .shape = {.position = {.x = xOffset + shape.halfSize.x +
                                           buttonHalfWidth + buttonMargin,
                                      .y = yOffset + height - buttonHalfHeight -
                                           buttonMargin},
                         .halfSize = {buttonHalfWidth, buttonHalfHeight}}};
  }

  void setOptions(std::vector<std::string> optionLabels) {
    options.clear();
    for (auto &labelText : optionLabels) {
      options.push_back(Button{.label = Label(labelText),
                               .shape = {.position = {}, .halfSize = {}}});
    }
    buildLayout(shape);
  }

  void open() { mode = OPEN; }

  void close() { mode = CLOSED; }

  void handleMouseDown(const vec2f_t &mousePosition) {
    int selected = 0;
    for (auto &option : options) {
      if (DoButtonClick(&option, mousePosition)) {
        selection = selected;
      }
      ++selected;
    }

    if (DoButtonClick(&cancelButton, mousePosition)) {
      close();
    }
    if (DoButtonClick(&selectButton, mousePosition)) {
      mode = SELECTED;
    }
  }
  void handleMouseUp(const vec2f_t &mousePosition) {}
  void draw(SDL_Renderer *renderer, const Style &style) {

    title.draw(style.inactiveColor, style.color1, renderer, style,
               HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    DrawFilledRect(shape, renderer, SDL_Color{0, 0, 0, 0});
    DrawFilledRect(title.shape, renderer, style.color1);
    title.draw(style.inactiveColor, style.color1, renderer, style,
               HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    DrawBoxOutline(shape, renderer, style.color1);

    for (auto &button : options) {
      DrawButton(&button, renderer, style);
    }

    DrawButton(&cancelButton, renderer, style);
    DrawButton(&selectButton, renderer, style);
  }
};
