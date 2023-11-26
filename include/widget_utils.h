#pragma once

#include "SDL_rect.h"
#include "collider.h"

static inline const SDL_Rect
ConvertAxisAlignedBoxToSDL_Rect(const AxisAlignedBoundingBox &box) {
  return SDL_Rect{.x = static_cast<int>(box.position.x - box.halfSize.x),
                  .y = static_cast<int>(box.position.y - box.halfSize.y),
                  .w = static_cast<int>(box.halfSize.x * 2),
                  .h = static_cast<int>(box.halfSize.y * 2)};
}
