#pragma once

#include "SDL_render.h"
#include "widget_button.h"
#include "widget_state.h"
#include <vector>

inline void DrawRadioButtonSingle(
    Button *button, SDL_Renderer *renderer, const Style &style,
    HorizontalAlignment horizontalAlignment = HorizontalAlignment::CENTER,
    VerticalAlignment verticalAlignment = VerticalAlignment::CENTER) {

  auto rect = SDL_Rect{.x = static_cast<int>(button->shape.position.x -
                                             button->shape.halfSize.x),
                       .y = static_cast<int>(button->shape.position.y -
                                             button->shape.halfSize.y),
                       .w = static_cast<int>(2 * button->shape.halfSize.x),
                       .h = static_cast<int>(2 * button->shape.halfSize.y)};

  auto backgroundColor =
      style.inactiveColor; // style.getWidgetColor(button.state);
  auto outlineColor =
      style.inactiveColor; // style.getWidgetColor(button.state);
  if (button->state == ACTIVE) {
    outlineColor = style.color0;
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  if (button->iconType != IconType::NONE) {
    auto texture = style.getIconTexture(button->iconType);
    auto color = style.getWidgetIconColor(button->state);
    auto backgroundColor = style.inactiveColor;
    DrawFilledRect(rect, renderer, backgroundColor);
    SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    auto iconRect = rect;
    iconRect.x = iconRect.x + (iconRect.w - iconRect.h) / 2;
    iconRect.w = iconRect.h;
    SDL_RenderCopy(renderer, texture, NULL, &iconRect);
  } else {
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g,
                           backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &rect);
    if (button->state == ACTIVE) {
      SDL_SetRenderDrawColor(renderer, style.color1.r, style.color1.g,
                             style.color1.b, style.color1.a);
      SDL_RenderFillRect(renderer, &rect);
    }

    button->label.draw(style.hoverColor, backgroundColor, rect, renderer, style,
                       horizontalAlignment, verticalAlignment);
  }
}

struct RadioGroup {
  int selectedIndex = 0;
  float buttonMargin = 10;
  float buttonHeight = 100;
  AxisAlignedBoundingBox shape;
  std::vector<Button> options;
  WidgetState state = INACTIVE;

  RadioGroup() {}

  RadioGroup(const std::vector<std::string> &optionLabels,
             const int initialSelection) {
    for (auto &label : optionLabels) {
      options.push_back(Button{
          .label = Label(label),
      });
    }
    selectedIndex = initialSelection;
  }

  inline void buildLayout(const AxisAlignedBoundingBox &bounds) {
    if (options.size() > 0) {
      shape = bounds;
      const float buttonWidth =
          (shape.halfSize.x * 2 - float(options.size() * buttonMargin)) /
          float(options.size());
      buttonHeight = shape.halfSize.y;
      for (size_t i = 0; i < options.size(); ++i) {
        options[i].shape = AxisAlignedBoundingBox{
            .position = vec2f_t{.x = static_cast<float>(
                                    shape.position.x -
                                    (buttonWidth + buttonMargin) *
                                        options.size() / 2.0 +
                                    (buttonWidth + buttonMargin) / 2 +
                                    i * (buttonWidth + buttonMargin)),
                                .y = static_cast<float>(shape.position.y)},
            .halfSize = vec2f_t{.x = static_cast<float>(buttonWidth / 2),
                                .y = static_cast<float>(shape.halfSize.y)}};

        options[i].state = INACTIVE;
      }
      options[selectedIndex].state = WidgetState::ACTIVE;
    }
  }

  inline void refreshLayout() { buildLayout(shape); }
};

inline void DrawRadioGroup(RadioGroup *group, SDL_Renderer *renderer,
                           const Style &style) {
  // DrawFilledRect(group->shape, renderer, style.color2);
  auto margin = group->shape.halfSize.x * 0.1;
  auto radiogroupBackgroundRect =
      SDL_Rect{.x = static_cast<int>(group->shape.position.x -
                                     group->shape.halfSize.x - margin),
               .y = static_cast<int>(group->shape.position.y -
                                     group->shape.halfSize.y - margin),
               .w = static_cast<int>(group->shape.halfSize.x * 2 + margin),
               .h = static_cast<int>(group->shape.halfSize.y * 2 + 2 * margin)};
  for (auto &button : group->options) {
    DrawRadioButtonSingle(&button, renderer, style);
  }
}

inline const bool DoClickRadioGroup(RadioGroup *group,
                                    const vec2f_t &mousePosition) {

  bool selectionChanged = false;

  auto selection = 0;
  for (auto &button : group->options) {
    if (DoButtonClick(&button, mousePosition)) {
      group->selectedIndex = selection;
      selectionChanged = true;
    } else {
      button.state = INACTIVE;
    }
    ++selection;
  }
  group->options[group->selectedIndex].state = ACTIVE;
  return selectionChanged;
}

inline void DoRadioGroupHover(RadioGroup *group, const vec2f_t &position) {
  for (auto &button : group->options) {
    DoButtonHover(&button, position);
  }
}
