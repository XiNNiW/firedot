#pragma once

#include "SDL_render.h"
#include "widget_button.h"
#include "widget_state.h"
#include "widget_style.h"
#include <vector>

inline void DrawDropDownButton(const Button &button, SDL_Renderer *renderer,
                               const Style &style) {

  auto rect = SDL_Rect{
      .x = static_cast<int>(button.shape.position.x - button.shape.halfSize.x),
      .y = static_cast<int>(button.shape.position.y - button.shape.halfSize.y),
      .w = static_cast<int>(2 * button.shape.halfSize.x),
      .h = static_cast<int>(2 * button.shape.halfSize.y)};

  auto backgroundColor =
      style.inactiveColor;              // style.getWidgetColor(button.state);
  auto outlineColor = style.hoverColor; // style.getWidgetColor(button.state);
  if (button.state == ACTIVE) {
    outlineColor = style.inactiveColor;
    backgroundColor = style.color1;
  }

  SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g,
                         backgroundColor.b, backgroundColor.a);

  SDL_RenderFillRect(renderer, &rect);

  SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g,
                         outlineColor.b, outlineColor.a);
  SDL_RenderDrawRect(renderer, &rect);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  if (button.labelText.size() > 0) {

    DrawLabel(button.labelText, style.hoverColor, backgroundColor, rect,
              renderer, style, HorizontalAlignment::CENTER);
  }
}

struct MultiSelectMenu {
  enum Action { NOTHING, MENU_OPEN_CLICKED, MENU_SELECTION_CHANGED };
  WidgetState state = INACTIVE;
  float width = 500, height = 500, topMargin = 50, sideMargin = 15,
        titleBarHeight = 100;
  size_t selected = 0;
  Button closeButton;
  Button menuButton;
  std::string menuLabel;
  std::vector<Button> options;

  inline void buildLayout(const float _width, const float _height) {
    width = _width;
    height = _height;
    rebuildLayout();
  }
  inline void rebuildLayout() {
    auto numOptions = options.size();
    auto menuHeight = height - 2 * topMargin - titleBarHeight;
    auto menuWidth = width - 2 * sideMargin;
    auto menuCenter = vec2f_t{.x = static_cast<float>(width / 2.0),
                              .y = static_cast<float>(height / 2.0)};

    auto closeButtonSize = vec2f_t{.x = static_cast<float>(width / 8.0),
                                   .y = static_cast<float>(titleBarHeight)};
    closeButton =
        Button{.labelText = "<- close",
               .shape = AxisAlignedBoundingBox{
                   .position = {.x = static_cast<float>(
                                    sideMargin + closeButtonSize.x / 2.0),
                                .y = static_cast<float>(topMargin)},
                   .halfSize = closeButtonSize.scale(0.5)}};

    for (size_t i = 0; i < numOptions; ++i) {
      auto buttonSize = vec2f_t{.x = menuWidth, .y = menuHeight / numOptions};
      auto buttonPosition = vec2f_t{
          .x = static_cast<float>(width / 2.0),
          .y = static_cast<float>((i * (buttonSize.y + 4)) + titleBarHeight +
                                  topMargin + buttonSize.y / 2.0)};
      options[i].shape = AxisAlignedBoundingBox{
          .position = buttonPosition, .halfSize = buttonSize.scale(0.5)};
      if (i == selected) {
        options[i].state = ACTIVE;
      }
    }
  }
  inline void setOptions(const std::vector<Button> &optionButtons) {
    options = optionButtons;
    rebuildLayout();
  }
};

inline static MultiSelectMenu::Action
DoMultiSelectClick(MultiSelectMenu *menu, const vec2f_t &position) {
  if (menu->state != ACTIVE && menu->menuButton.shape.contains(position)) {
    menu->state = ACTIVE;
    return MultiSelectMenu::MENU_OPEN_CLICKED;
  } else if (menu->state == ACTIVE) {

    if (DoButtonClick(&menu->closeButton, position)) {
      menu->state = INACTIVE;
      return MultiSelectMenu::NOTHING;
    }

    size_t selected = 0;
    auto previousSelection = menu->selected;
    for (auto &button : menu->options) {

      if (DoButtonClick(&button, position)) {
        menu->selected = selected;
        break;
      }
      ++selected;
    }
    if (menu->selected != previousSelection) {
      for (size_t i = 0; i < menu->options.size(); ++i) {
        if (i == menu->selected) {
          menu->options[i].state = ACTIVE;
        } else {
          menu->options[i].state = INACTIVE;
        }
      }
      return MultiSelectMenu::MENU_SELECTION_CHANGED;
    }
  }
  return MultiSelectMenu::NOTHING;
}

inline static void DrawMultiSelectMenu(const MultiSelectMenu &menu,
                                       SDL_Renderer *renderer,
                                       const Style &style) {
  if (menu.state == ACTIVE) {

    auto screenRect = SDL_Rect{.x = 0,
                               .y = 0,
                               .w = static_cast<int>(menu.width),
                               .h = static_cast<int>(menu.height)};

    SDL_SetRenderDrawColor(renderer, style.inactiveColor.r,
                           style.inactiveColor.g, style.inactiveColor.b, 0xb0);
    SDL_RenderFillRect(renderer, &screenRect);

    DrawButton(menu.closeButton, renderer, style);
    for (auto &button : menu.options) {
      DrawDropDownButton(button, renderer, style);
    }
    auto titleRect = screenRect;
    titleRect.h = menu.titleBarHeight;
    DrawLabel(menu.menuButton.labelText, style.inactiveColor, style.hoverColor,
              titleRect, renderer, style, HorizontalAlignment::CENTER);
  } else {
    DrawButton(menu.menuButton, renderer, style);
  }
}
