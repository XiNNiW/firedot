#pragma once

#include "SDL_render.h"
#include "collider.h"
#include "widget_style.h"
#include "widget_utils.h"

struct Icon {
  IconType type;
  SDL_Rect srcRect;
  AxisAlignedBoundingBox shape;
};

inline void DrawIcon(Icon *icon, SDL_Renderer *renderer, const Style &style,
                     const SDL_Color &color) {

  auto destRect = ConvertAxisAlignedBoxToSDL_Rect(icon->shape);
  SDL_RenderCopy(renderer, style.getIconTexture(icon->type), &icon->srcRect,
                 &destRect);
}
