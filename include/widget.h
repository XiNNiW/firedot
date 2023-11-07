#pragma once

#include "SDL_ttf.h"
#include "collider.h"
#include "vector_math.h"
#include <string>
#include <vector>
enum UIState { INACTIVE, HOVER, ACTIVE };
enum Alignment { LEFT, CENTER };
struct Button {
  std::string labelText = "";
  AxisAlignedBoundingBox shape =
      AxisAlignedBoundingBox{.position = {0, 0}, .halfSize = {100, 100}};
  UIState state = INACTIVE;
};
static inline const SDL_Rect
ConvertAxisAlignedBoxToSDL_Rect(const AxisAlignedBoundingBox &box) {
  return SDL_Rect{.x = static_cast<int>(box.position.x - box.halfSize.x),
                  .y = static_cast<int>(box.position.y - box.halfSize.y),
                  .w = static_cast<int>(box.halfSize.x * 2),
                  .h = static_cast<int>(box.halfSize.y * 2)};
}
inline const bool DoButtonClick(Button *button, const vec2f_t mousePosition,
                                const UIState newState) {
  if (button->shape.contains(mousePosition)) {
    button->state = newState;
    return true;
  };
  return false;
}

inline const bool DoButtonClick(Button *button, const vec2f_t mousePosition) {
  if (button->shape.contains(mousePosition)) {
    return true;
  };
  return false;
}

static bool DoButtonHover(Button *button, const vec2f_t &position) {
  if (button->shape.contains(position)) {
    if ((button->state != UIState::ACTIVE))
      button->state = UIState::HOVER;
    return true;
  } else {
    if ((button->state != UIState::ACTIVE))
      button->state = UIState::INACTIVE;
  }
  return false;
}
struct Style {
  TTF_Font *font;

  SDL_Color color0 = SDL_Color{.r = 0xd6, .g = 0x02, .b = 0x70, .a = 0xff};
  SDL_Color color1 = SDL_Color{.r = 0x9b, .g = 0x4f, .b = 0x96, .a = 0xff};
  SDL_Color color2 = SDL_Color{.r = 0x00, .g = 0x38, .b = 0xa8, .a = 0xff};
  SDL_Color inactiveColor =
      SDL_Color{.r = 0x1b, .g = 0x1b, .b = 0x1b, .a = 0xff};
  SDL_Color hoverColor = SDL_Color{.r = 0xa0, .g = 0xa0, .b = 0xa0, .a = 0xff};

  inline const SDL_Color getWidgetColor(const UIState state) const {

    switch (state) {
    case INACTIVE:
      return inactiveColor;
      break;
    case HOVER:
      return hoverColor;
      break;
    case ACTIVE:
      return color0;
      break;
    }
    return SDL_Color();
  }

  inline const SDL_Color getWidgetLabelColor(const UIState state) const {

    switch (state) {
    case INACTIVE:
      return hoverColor;
      break;
    case HOVER:
      return color1;
      break;
    case ACTIVE:
      return inactiveColor;
      break;
    }
    return SDL_Color();
  }
};

inline const void DrawButton(const Button &button, SDL_Texture *buttonTexture,
                             SDL_Renderer *renderer, const Style &style) {
  auto destRect = SDL_Rect{
      .x = static_cast<int>(button.shape.position.x - button.shape.halfSize.x),
      .y = static_cast<int>(button.shape.position.y - button.shape.halfSize.y),
      .w = static_cast<int>(2 * button.shape.halfSize.x),
      .h = static_cast<int>(2 * button.shape.halfSize.y)};

  SDL_Point center = SDL_Point{.x = static_cast<int>(button.shape.position.x),
                               .y = static_cast<int>(button.shape.position.y)};
  auto color = style.getWidgetColor(button.state);
  SDL_SetTextureColorMod(buttonTexture, color.r, color.g, color.b);

  SDL_RenderCopy(renderer, buttonTexture, NULL, &destRect);
}

inline const void DrawLabel(const std::string &text, const SDL_Color &textColor,
                            const SDL_Color &backgroundColor,
                            const SDL_Rect &labelBox, SDL_Renderer *renderer,
                            const Style &style,
                            const Alignment alignment = LEFT) {

  auto labelText = text.c_str();

  if (text.length() > 0) {

    auto textSurface =
        TTF_RenderUTF8_LCD(style.font, labelText, textColor, backgroundColor);

    if (textSurface != NULL) {
      auto textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
      SDL_Rect textSrcRect =
          SDL_Rect{.x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h};

      auto textDestRect = textSrcRect;
      textDestRect.x = labelBox.x;
      textDestRect.y = labelBox.y;
      switch (alignment) {
      case LEFT:
        break;
      case CENTER: {
        textDestRect.x += labelBox.w / 2 - textSrcRect.w / 2;
        textDestRect.y += labelBox.h / 2 - textSrcRect.h / 2;
        break;
      }
      }
      SDL_RenderCopy(renderer, textTexture, &textSrcRect, &textDestRect);
      SDL_FreeSurface(textSurface);
      SDL_DestroyTexture(textTexture);
    }
  }
}

// inline const SDL_Color GetWidgetLabelColor(const UIState state,
//                                            const Style &style) {
//   auto textColor = style.hoverColor;
//   if (state == ACTIVE) {
//     textColor = style.inactiveColor;
//   } else if (state == HOVER) {
//     textColor = style.color1;
//   }
//   return textColor;
// }

inline const void DrawButtonLabel(const Button &button, SDL_Renderer *renderer,
                                  const Style &style,
                                  const Alignment alignment = CENTER) {

  auto textColor = style.getWidgetLabelColor(button.state);
  auto color = style.getWidgetColor(button.state);
  DrawLabel(button.labelText, textColor, color,
            ConvertAxisAlignedBoxToSDL_Rect(button.shape), renderer, style,
            alignment);
}
inline const void DrawButton(const Button &button, SDL_Renderer *renderer,
                             const Style &style) {
  auto rect = SDL_Rect{
      .x = static_cast<int>(button.shape.position.x - button.shape.halfSize.x),
      .y = static_cast<int>(button.shape.position.y - button.shape.halfSize.y),
      .w = static_cast<int>(2 * button.shape.halfSize.x),
      .h = static_cast<int>(2 * button.shape.halfSize.y)};
  auto color = style.getWidgetColor(button.state);

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

  SDL_RenderFillRect(renderer, &rect);
  SDL_SetRenderDrawColor(renderer, style.color2.r, style.color2.g,
                         style.color2.b, style.color2.a);
  SDL_RenderDrawRect(renderer, &rect);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  if (button.labelText.size() > 0) {

    DrawButtonLabel(button, renderer, style);
  }
}
inline void DrawRadioButtonSingle(const Button &button, SDL_Renderer *renderer,
                                  const Style &style) {

  auto rect = SDL_Rect{
      .x = static_cast<int>(button.shape.position.x - button.shape.halfSize.x),
      .y = static_cast<int>(button.shape.position.y - button.shape.halfSize.y),
      .w = static_cast<int>(2 * button.shape.halfSize.x),
      .h = static_cast<int>(2 * button.shape.halfSize.y)};

  auto backgroundColor =
      style.inactiveColor; // style.getWidgetColor(button.state);
  auto outlineColor =
      style.inactiveColor; // style.getWidgetColor(button.state);
  if (button.state == ACTIVE) {
    outlineColor = style.color0;
  }

  SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g,
                         backgroundColor.b, backgroundColor.a);

  SDL_RenderFillRect(renderer, &rect);
  //  SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g,
  //                         outlineColor.b, outlineColor.a);
  //  SDL_RenderDrawRect(renderer, &rect);

  if (button.state == ACTIVE) {
    //  auto smallerRectAABB = button.shape;
    // smallerRectAABB.halfSize = smallerRectAABB.halfSize.scale(0.33);
    // auto smallerRect = ConvertAxisAlignedBoxToSDL_Rect(smallerRectAABB);
    SDL_SetRenderDrawColor(renderer, style.color1.r, style.color1.g,
                           style.color1.b, style.color1.a);
    SDL_RenderFillRect(renderer, &rect);
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  if (button.labelText.size() > 0) {

    DrawLabel(button.labelText, style.hoverColor, backgroundColor, rect,
              renderer, style);
  }
}

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
              renderer, style, Alignment::CENTER);
  }
}

struct MultiSelectMenu {
  enum Action { NOTHING, MENU_OPEN_CLICKED, MENU_SELECTION_CHANGED };
  UIState state = INACTIVE;
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
              titleRect, renderer, style, CENTER);
  } else {
    DrawButton(menu.menuButton, renderer, style);
  }
}

struct RadioGroup {
  int selectedIndex = 0;
  float buttonMargin = 10;
  float buttonHeight = 100;
  AxisAlignedBoundingBox shape;
  std::vector<Button> options = std::vector<Button>();

  static const RadioGroup
  MakeRadioGroup(const std::vector<std::string> &optionLabels,
                 const int initialSelection) {
    auto group = RadioGroup{
        .selectedIndex = initialSelection,
    };
    for (auto &label : optionLabels) {
      group.options.push_back(Button{
          .labelText = label,
      });
    }
    return group;
  }

  inline void buildLayout(const AxisAlignedBoundingBox &bounds) {
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
    options[selectedIndex].state = UIState::ACTIVE;
  }
};

inline void DrawRadioGroup(const RadioGroup &group, SDL_Renderer *renderer,
                           const Style &style) {
  auto radiogroupBackgroundRect = SDL_Rect{
      .x =
          static_cast<int>(group.shape.position.x - group.shape.halfSize.x - 5),
      .y =
          static_cast<int>(group.shape.position.y - group.shape.halfSize.y - 5),
      .w = static_cast<int>(group.shape.halfSize.x * 2 + 5),
      .h = static_cast<int>(group.shape.halfSize.y * 2 + 10)};
  //  SDL_SetRenderDrawColor(renderer, style.color2.r, style.color2.g,
  //                         style.color2.b, style.color2.a);
  //  SDL_RenderFillRect(renderer, &radiogroupBackgroundRect);
  for (auto &button : group.options) {
    // DrawButton(button, renderer, style);
    DrawRadioButtonSingle(button, renderer, style);
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

struct HSlider {
  UIState state = INACTIVE;
  std::string labelText;
  float value;
  AxisAlignedBoundingBox shape;
};

inline void SetHSliderValue(HSlider *slider, const vec2f_t &mousePosition) {
  float width = slider->shape.halfSize.x * 2;
  float relativePosition =
      mousePosition.x - (slider->shape.position.x - slider->shape.halfSize.x);
  SDL_Log("slider %f/%f", relativePosition, width);
  slider->value = fmax(fmin(relativePosition / width, 1.0), 0.0);
}

inline bool DoHSliderClick(HSlider *slider, const vec2f_t &mousePosition) {
  if (slider->shape.contains(mousePosition)) {
    SetHSliderValue(slider, mousePosition);
    slider->state = ACTIVE;
    return true;
  }
  return false;
}

inline bool DoHSliderDrag(HSlider *slider, const vec2f_t &mousePosition) {
  if (slider->state == ACTIVE) {
    SetHSliderValue(slider, mousePosition);
    return true;
  }
  return false;
}

inline void DrawHSlider(const HSlider &slider, SDL_Renderer *renderer,
                        const Style &style) {
  auto sliderBounds = ConvertAxisAlignedBoxToSDL_Rect(slider.shape);
  auto dataBounds = sliderBounds;
  dataBounds.w *= slider.value;
  auto valueColor = style.color1;
  if (slider.state == ACTIVE) {
    valueColor = style.color0;
  }
  SDL_SetRenderDrawColor(renderer, style.inactiveColor.r, style.inactiveColor.g,
                         style.inactiveColor.b, style.inactiveColor.a);

  SDL_RenderFillRect(renderer, &sliderBounds);
  SDL_SetRenderDrawColor(renderer, valueColor.r, valueColor.g, valueColor.b,
                         valueColor.a);
  SDL_RenderFillRect(renderer, &dataBounds);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

  DrawLabel(slider.labelText, style.getWidgetLabelColor(slider.state),
            style.getWidgetColor(slider.state), sliderBounds, renderer, style);
}
