#pragma once

#include "SDL_ttf.h"
#include "widget_state.h"

struct Style {
  TTF_Font *font;

  SDL_Color color0 = SDL_Color{.r = 0xd6, .g = 0x02, .b = 0x70, .a = 0xff};
  SDL_Color color1 = SDL_Color{.r = 0x9b, .g = 0x4f, .b = 0x96, .a = 0xff};
  SDL_Color color2 = SDL_Color{.r = 0x00, .g = 0x38, .b = 0xa8, .a = 0xff};
  SDL_Color inactiveColor =
      SDL_Color{.r = 0x1b, .g = 0x1b, .b = 0x1b, .a = 0xff};
  SDL_Color hoverColor = SDL_Color{.r = 0xa0, .g = 0xa0, .b = 0xa0, .a = 0xff};
  SDL_Color unavailableColor =
      SDL_Color{.r = 0x2b, .g = 0x2b, .b = 0x2b, .a = 0xff};

  inline const SDL_Color getWidgetColor(const WidgetState state) const {

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
    case HIDDEN:
      break;
    }
    return SDL_Color();
  }

  inline const SDL_Color getWidgetLabelColor(const WidgetState state) const {

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
    case HIDDEN:
      break;
    }
    return SDL_Color();
  }
};
