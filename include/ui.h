#pragma once

#include "SDL_ttf.h"
#include "collider.h"
#include <string>
enum UIState { INACTIVE, HOVER, ACTIVE };
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

static void DoButtonHover(Button *button, const vec2f_t &position) {
  if ((button->state != UIState::ACTIVE) && (button->state != UIState::HOVER) &&
      button->shape.contains(position)) {
    button->state = UIState::HOVER;
  }
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
      return color2;
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
                            const Style &style) {

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

      SDL_RenderCopy(renderer, textTexture, &textSrcRect, &textDestRect);
      SDL_FreeSurface(textSurface);
      SDL_DestroyTexture(textTexture);
    }
  }
}
inline const void DrawButtonLabel(const Button &button, SDL_Renderer *renderer,
                                  const Style &style) {

  auto textColor = style.getWidgetLabelColor(button.state);
  auto color = style.getWidgetColor(button.state);
  DrawLabel(button.labelText, textColor, color,
            ConvertAxisAlignedBoxToSDL_Rect(button.shape), renderer, style);
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

  // SDL_RenderDrawRect(renderer, &rect);
  SDL_RenderFillRect(renderer, &rect);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  if (button.labelText.size() > 0) {

    DrawButtonLabel(button, renderer, style);
  }
}
