#pragma once

#include "collider.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_radio_button.h"
#include "widget_state.h"
#include <vector>
struct OptionPopupUI {
  enum Mode { OPEN, CLOSED } mode = CLOSED;
  Label title = Label("");
  std::vector<Button> options = {};
  Button cancelButton;
  Button selectButton;
  AxisAlignedBoundingBox shape;
  AxisAlignedBoundingBox modalBackgroundShape;
  void buildLayout(AxisAlignedBoundingBox shape) {
    this->shape = shape;
    modalBackgroundShape = shape;
    modalBackgroundShape.halfSize = shape.halfSize.scale(10);

    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;

    auto margin = shape.halfSize.y / 32;

    title.shape = {
        .position = {.x = shape.position.x, .y = yOffset + height / 50},
        .halfSize = {.x = shape.halfSize.x, .y = height / 50}};
    auto actionButtonMargin = shape.halfSize.x / 24;
    auto actionButtonHalfWidth = shape.halfSize.x / 2 - actionButtonMargin;
    auto actionButtonHalfHeight = shape.halfSize.y / 8 - actionButtonMargin;
    float numButtons = options.size();
    vec2f_t buttonHalfSize = {
        .x = width / 3,
        .y = static_cast<float>((shape.halfSize.y - actionButtonHalfHeight * 2 -
                                 numButtons * margin + margin) /
                                numButtons)};
    int buttonY = title.shape.position.y + buttonHalfSize.y +
                  title.shape.halfSize.y + 2 * margin;

    for (auto &optionButton : options) {
      optionButton.shape = {.position = {.x = xOffset + shape.halfSize.x,
                                         .y = static_cast<float>(buttonY)},
                            .halfSize = buttonHalfSize};
      buttonY += buttonHalfSize.y * 2 + margin;
    }

    cancelButton = Button{
        .label = Label("cancel"),
        .shape = {.position = {.x = xOffset + shape.halfSize.x -
                                    actionButtonHalfWidth - actionButtonMargin,
                               .y = yOffset + height - actionButtonHalfHeight -
                                    actionButtonMargin},
                  .halfSize = {actionButtonHalfWidth, actionButtonHalfHeight}}};
    selectButton = Button{
        .label = Label("select"),
        .shape = {.position = {.x = xOffset + shape.halfSize.x +
                                    actionButtonHalfWidth + actionButtonMargin,
                               .y = yOffset + height - actionButtonHalfHeight -
                                    actionButtonMargin},
                  .halfSize = {actionButtonHalfWidth, actionButtonHalfHeight}}};
  }

  void open(const std::vector<std::string> &optionLabels,
            const int &selection) {
    options.clear();
    for (int i = 0; i < optionLabels.size(); ++i) {
      auto state = (selection == i) ? ACTIVE : INACTIVE;
      options.push_back(
          Button{.label = Label(optionLabels[i]), .state = state});
    }
    buildLayout(shape);
    mode = OPEN;
  }

  void close() { mode = CLOSED; }

  bool doClick(const vec2f_t &mousePosition, int *selection) {
    if (mode == OPEN) {
      bool selectionChanged = false;
      int selected = 0;
      for (int i = 0; i < options.size(); ++i) {
        if (DoButtonClick(&options[i], mousePosition)) {
          selectionChanged = true;
          selected = i;
        }
      }
      if (selectionChanged)
        updateSelection(selected);

      if (DoButtonClick(&cancelButton, mousePosition)) {
        close();
        return false;
      }
      if (DoButtonClick(&selectButton, mousePosition)) {
        close();
        *selection = getCurrentSelection();
        return true;
      }
    }
    return false;
  }

  bool isOpen() { return mode == OPEN; }

  void updateSelection(int selection) {
    for (int i = 0; i < options.size(); ++i) {
      if (selection == i) {
        options[i].state = ACTIVE;
      } else {
        options[i].state = INACTIVE;
      }
    }
  }

  int getCurrentSelection() const {
    for (int i = 0; i < options.size(); ++i) {
      if (options[i].state == ACTIVE) {
        return i;
      }
    }
    return 0;
  }

  void draw(SDL_Renderer *renderer, const Style &style) {
    if (mode == OPEN) {
      DrawFilledRect(modalBackgroundShape, renderer,
                     SDL_Color{.r = 0, .g = 0, .b = 0, .a = 128});

      title.draw(style.inactiveColor, style.color1, renderer, style,
                 HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
      DrawFilledRect(shape, renderer, SDL_Color{0, 0, 0, 0});
      DrawFilledRect(title.shape, renderer, style.color1);
      title.draw(style.inactiveColor, style.color1, renderer, style,
                 HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
      DrawBoxOutline(shape, renderer, style.color1);

      for (auto &button : options) {
        DrawRadioButtonSingle(&button, renderer, style,
                              HorizontalAlignment::CENTER);
      }

      DrawButton(&cancelButton, renderer, style);
      DrawButton(&selectButton, renderer, style);
    }
  }
};
