#pragma once

#include "SDL_render.h"
#include "widget_button.h"
#include <vector>

inline void DrawRadioButtonSingle(Button *button, SDL_Renderer *renderer,
                                  const Style &style) {

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

  SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g,
                         backgroundColor.b, backgroundColor.a);

  SDL_RenderFillRect(renderer, &rect);
  //  SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g,
  //                         outlineColor.b, outlineColor.a);
  //  SDL_RenderDrawRect(renderer, &rect);

  if (button->state == ACTIVE) {
    //  auto smallerRectAABB = button.shape;
    // smallerRectAABB.halfSize = smallerRectAABB.halfSize.scale(0.33);
    // auto smallerRect = ConvertAxisAlignedBoxToSDL_Rect(smallerRectAABB);
    SDL_SetRenderDrawColor(renderer, style.color1.r, style.color1.g,
                           style.color1.b, style.color1.a);
    SDL_RenderFillRect(renderer, &rect);
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  if (button->label.getText().size() > 0) {

    button->label.draw(style.hoverColor, backgroundColor, rect, renderer,
                       style);
  }
}

struct RadioGroup {
  int selectedIndex = 0;
  float buttonMargin = 10;
  float buttonHeight = 100;
  AxisAlignedBoundingBox shape;
  std::vector<Button> options;

  RadioGroup() {}

  RadioGroup(const std::vector<std::string> &optionLabels,
             const int initialSelection) {
    for (auto &label : optionLabels) {
      options.push_back(Button{
          .label = Label(label),
      });
    }
  }

  inline void buildLayout(const AxisAlignedBoundingBox &bounds) {
    if (options.size() > 0) {
      shape = bounds;
      const size_t initialSynthTypeSelection = 0;
      const float buttonWidth =
          (shape.halfSize.x * 2 - float(options.size() * buttonMargin)) /
          float(options.size());
      buttonHeight = shape.halfSize.y;
      for (size_t i = 0; i < options.size(); ++i) {
        options[i].shape = AxisAlignedBoundingBox{
            .position =
                vec2f_t{.x = static_cast<float>(
                            shape.position.x - shape.halfSize.x +
                            buttonWidth / 2 + i * (buttonWidth + buttonMargin)),
                        .y = static_cast<float>(shape.position.y)},
            .halfSize = vec2f_t{.x = static_cast<float>(buttonWidth / 2),
                                .y = static_cast<float>(shape.halfSize.y)}};

        options[i].state = INACTIVE;
      }
      options[selectedIndex].state = WidgetState::ACTIVE;
    }
  }
};

inline void DrawRadioGroup(RadioGroup *group, SDL_Renderer *renderer,
                           const Style &style) {
  auto radiogroupBackgroundRect =
      SDL_Rect{.x = static_cast<int>(group->shape.position.x -
                                     group->shape.halfSize.x - 5),
               .y = static_cast<int>(group->shape.position.y -
                                     group->shape.halfSize.y - 5),
               .w = static_cast<int>(group->shape.halfSize.x * 2 + 5),
               .h = static_cast<int>(group->shape.halfSize.y * 2 + 10)};
  //  SDL_SetRenderDrawColor(renderer, style.color2.r, style.color2.g,
  //                         style.color2.b, style.color2.a);
  //  SDL_RenderFillRect(renderer, &radiogroupBackgroundRect);
  for (auto &button : group->options) {
    // DrawButton(button, renderer, style);
    DrawRadioButtonSingle(&button, renderer, style);
  }
}
inline const bool DoClickRadioGroup(RadioGroup *group, /* int *selectedIndex,*/
                                    const vec2f_t &mousePosition) {

  bool selectionChanged = false;

  auto selection = 0;
  for (auto &button : group->options) {
    if (DoButtonClick(&button, mousePosition)) {
      group->selectedIndex = selection;
      //*selectedIndex = selection;
      selectionChanged = true;
    } else {
      button.state = INACTIVE;
    }
    ++selection;
  }
  // group->options[*selectedIndex].state = ACTIVE;
  group->options[group->selectedIndex].state = ACTIVE;
  return selectionChanged;
}

inline void DoRadioGroupHover(RadioGroup *group, const vec2f_t &position) {
  for (auto &button : group->options) {
    DoButtonHover(&button, position);
  }
}
