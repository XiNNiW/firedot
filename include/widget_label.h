#pragma once

#include "SDL_render.h"
#include "widget_style.h"
#include <string>

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
