#pragma once
#include "SDL_rect.h"
#include "SDL_render.h"
#include "collider.h"
#include "widget_icon.h"
#include "widget_label.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_utils.h"
#include <string>
inline void DrawFilledRect(const SDL_Rect &rect, SDL_Renderer *renderer,
                           const SDL_Color &color) {

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer, &rect);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
}
inline void DrawFilledRect(const AxisAlignedBoundingBox &shape,
                           SDL_Renderer *renderer, const SDL_Color &color) {
  auto rect =
      SDL_Rect{.x = static_cast<int>(shape.position.x - shape.halfSize.x),
               .y = static_cast<int>(shape.position.y - shape.halfSize.y),
               .w = static_cast<int>(2 * shape.halfSize.x),
               .h = static_cast<int>(2 * shape.halfSize.y)};

  DrawFilledRect(rect, renderer, color);
}

inline void DrawBoxOutline(const SDL_Rect &rect, SDL_Renderer *renderer,
                           const SDL_Color &color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderDrawRect(renderer, &rect);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
}

inline void DrawBoxOutline(const AxisAlignedBoundingBox &shape,
                           SDL_Renderer *renderer, const SDL_Color &color) {
  DrawBoxOutline(ConvertAxisAlignedBoxToSDL_Rect(shape), renderer, color);
}

struct Button {
  // std::string labelText = "";
  Label label = Label();
  AxisAlignedBoundingBox shape =
      AxisAlignedBoundingBox{.position = {0, 0}, .halfSize = {100, 100}};
  WidgetState state = INACTIVE;
  IconType iconType = IconType::NONE;
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

  auto color = style.getWidgetColor(button.state);
  SDL_SetTextureColorMod(buttonTexture, color.r, color.g, color.b);

  SDL_RenderCopy(renderer, buttonTexture, NULL, &destRect);
}

inline const void DrawButtonLabel(
    Button *button, SDL_Renderer *renderer, const Style &style,
    const HorizontalAlignment horizontalAlignment = HorizontalAlignment::CENTER,
    const VerticalAlignment verticalAlignment = VerticalAlignment::CENTER) {

  auto textColor = style.getWidgetLabelColor(button->state);
  auto color = style.getWidgetColor(button->state);
  button->label.draw(textColor, color,
                     ConvertAxisAlignedBoxToSDL_Rect(button->shape), renderer,
                     style, horizontalAlignment, verticalAlignment);
}
inline const void DrawButton(Button *button, SDL_Renderer *renderer,
                             const Style &style, SDL_Color borderColor) {

  if (button->state != HIDDEN) {
    auto rect = ConvertAxisAlignedBoxToSDL_Rect(button->shape);
    if (button->iconType != IconType::NONE) {
      auto color = style.getWidgetIconColor(button->state);
      auto texture = style.getIconTexture(button->iconType);

      if (texture != NULL) {
        SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
        rect.x = rect.x + (rect.w - rect.h) / 2;
        rect.w = rect.h;
        SDL_RenderCopy(renderer, texture, NULL, &rect);
      }
    } else {
      auto color = style.getWidgetColor(button->state);
      DrawFilledRect(rect, renderer, color);
      DrawBoxOutline(rect, renderer, borderColor);
      if (button->label.getText().size() > 0) {
        DrawButtonLabel(button, renderer, style);
      }
    }
  }
}

inline const void DrawButton(Button *button, SDL_Renderer *renderer,
                             const Style &style) {

  DrawButton(button, renderer, style, style.color2);
}
