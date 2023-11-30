#pragma once

#include "SDL_pixels.h"
#include "SDL_render.h"
#include "vector_math.h"
inline void DrawLine(vec2f_t start, vec2f_t end, SDL_Renderer *renderer,
                     const SDL_Color &color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderDrawLine(renderer, start.x, start.y, end.x, end.y);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
}
