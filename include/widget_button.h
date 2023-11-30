#pragma once
#include "SDL_rect.h"
#include "SDL_render.h"
#include "collider.h"
#include "widget_label.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_utils.h"
#include <string>

struct Button {
  std::string labelText = "";
  AxisAlignedBoundingBox shape =
      AxisAlignedBoundingBox{.position = {0, 0}, .halfSize = {100, 100}};
  WidgetState state = INACTIVE;
};

inline const bool DoButtonClick(Button *button, const vec2f_t mousePosition,
                                const WidgetState newState) {
  if (button->shape.contains(mousePosition)) {
    button->state = newState;
    return true;
  };
  return false;
}

inline const bool DoButtonClick(Button *button, const vec2f_t mousePosition) {

  if ((button->state != HIDDEN) && button->shape.contains(mousePosition)) {
    return true;
  };
  return false;
}

static bool DoButtonHover(Button *button, const vec2f_t &position) {
  if (button->shape.contains(position)) {
    if ((button->state != WidgetState::ACTIVE))
      button->state = WidgetState::HOVER;
    return true;
  } else {
    if ((button->state != WidgetState::ACTIVE))
      button->state = WidgetState::INACTIVE;
  }
  return false;
}

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

inline const void DrawButtonLabel(
    const Button &button, SDL_Renderer *renderer, const Style &style,
    const HorizontalAlignment horizontalAlignment = HorizontalAlignment::CENTER,
    const VerticalAlignment verticalAlignment = VerticalAlignment::CENTER) {

  auto textColor = style.getWidgetLabelColor(button.state);
  auto color = style.getWidgetColor(button.state);
  DrawLabel(button.labelText, textColor, color,
            ConvertAxisAlignedBoxToSDL_Rect(button.shape), renderer, style,
            horizontalAlignment, verticalAlignment);
}
inline const void DrawButton(const Button &button, SDL_Renderer *renderer,
                             const Style &style) {

  if (button.state != HIDDEN) {
    auto rect = SDL_Rect{.x = static_cast<int>(button.shape.position.x -
                                               button.shape.halfSize.x),
                         .y = static_cast<int>(button.shape.position.y -
                                               button.shape.halfSize.y),
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
}
