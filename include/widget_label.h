#pragma once

#include "SDL_log.h"
#include "SDL_render.h"
#include "collider.h"
#include "vector_math.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_utils.h"
#include <memory>
#include <string>

class Label {
private:
  std::string text = "";
  SDL_Texture *cachedTexture = NULL;
  SDL_Rect cachedSourceRect;

public:
  AxisAlignedBoundingBox shape;
  Label() {}
  Label(std::string _text) : text(_text) {}
  inline void setText(std::string text) {
    this->text = text;
    if (cachedTexture != NULL) {
      SDL_DestroyTexture(cachedTexture);
      cachedTexture = NULL;
    }
  }

  ~Label() {
    if (cachedTexture != NULL) {
      SDL_DestroyTexture(cachedTexture);
      cachedTexture = NULL;
    }
  }

  inline const std::string &getText() { return text; }

  inline const void draw(
      const SDL_Color &textColor, const SDL_Color &backgroundColor,
      SDL_Renderer *renderer, const Style &style,
      const HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT,
      const VerticalAlignment verticalAlignment = VerticalAlignment::TOP) {
    draw(textColor, backgroundColor, ConvertAxisAlignedBoxToSDL_Rect(shape),
         renderer, style, horizontalAlignment, verticalAlignment);
  }

  inline const void draw(
      const SDL_Color &textColor, const SDL_Color &backgroundColor,
      const SDL_Rect &labelBox, SDL_Renderer *renderer, const Style &style,
      const HorizontalAlignment horizontalAlignment = HorizontalAlignment::LEFT,
      const VerticalAlignment verticalAlignment = VerticalAlignment::TOP) {

    auto labelText = text.c_str();

    if (text.length() > 0) {

      if (cachedTexture == NULL) {
        auto font = style.getFont();
        // SDL_LogInfo(0, "font %d", font);
        auto textSurface =
            TTF_RenderUTF8_LCD(font, labelText, textColor, backgroundColor);
        if (textSurface != NULL) {
          cachedSourceRect = SDL_Rect{
              .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h};
          cachedTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
          SDL_FreeSurface(textSurface);
        }
      }
      if (cachedTexture != NULL) {

        const auto &textTexture = cachedTexture;
        const auto &textSrcRect = cachedSourceRect;

        auto textDestRect = textSrcRect;
        textDestRect.x = labelBox.x;
        textDestRect.y = labelBox.y;
        switch (horizontalAlignment) {
        case HorizontalAlignment::LEFT:
          break;
        case HorizontalAlignment::CENTER: {
          textDestRect.x += labelBox.w / 2 - textSrcRect.w / 2;
          break;
        }
        }
        switch (verticalAlignment) {

        case VerticalAlignment::TOP:
          break;
        case VerticalAlignment::CENTER:
          textDestRect.y += labelBox.h / 2 - textSrcRect.h / 2;
          break;
        }
        SDL_RenderCopy(renderer, textTexture, &textSrcRect, &textDestRect);
        // SDL_FreeSurface(textSurface);
        // SDL_DestroyTexture(textTexture);
      }
    }
  }
};
