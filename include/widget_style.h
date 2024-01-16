#pragma once

#include "SDL_image.h"
#include "SDL_render.h"
#include "SDL_ttf.h"
#include "widget_state.h"
#include <string>

enum class IconType {
  GUITAR,
  CASSETTE,
  PLAY,
  STOP,
  GRAPH,
  GRID,
  GAME_PAD,
  SINE_WAVE,
  PIANO,
  DRUM,
  SAVE,
  NOTES,
  MAP,
  SLIDERS,
  RIGHT_ARROW,
  LEFT_ARROW,
  GEAR,
  TOUCH,
  NONE
};
static const size_t NUM_ICONS = 18;
static_assert(NUM_ICONS == static_cast<size_t>(IconType::NONE),
              "IconType enum and size must agree");
static const IconType iconTypes[NUM_ICONS] = {
    IconType::GUITAR,     IconType::CASSETTE,  IconType::PLAY,
    IconType::STOP,       IconType::GRAPH,     IconType::GRID,
    IconType::GAME_PAD,   IconType::SINE_WAVE, IconType::PIANO,
    IconType::DRUM,       IconType::SAVE,      IconType::NOTES,
    IconType::MAP,        IconType::SLIDERS,   IconType::RIGHT_ARROW,
    IconType::LEFT_ARROW, IconType::GEAR,      IconType::TOUCH};
static const char *iconPaths[NUM_ICONS] = {
    "images/guitar-svgrepo-com.svg",
    "images/cassette-tape-svgrepo-com.svg",
    "images/circle-play-svgrepo-com.svg",
    "images/circle-stop-svgrepo-com.svg",
    "images/diagram-project-svgrepo-com.svg",
    "images/grid-svgrepo-com.svg",
    "images/gamepad-svgrepo-com.svg",
    "images/colour-tuneing-svgrepo-com.svg",
    "images/piano-svgrepo-com.svg",
    "images/drum-svgrepo-com.svg",
    "images/save-svgrepo-com(2).svg",
    "images/music-notes-svgrepo-com.svg",
    "images/map-svgrepo-com.svg",
    "images/tuning-3-svgrepo-com.svg",
    "images/arrow-right-svgrepo-com.svg",
    "images/arrow-left-svgrepo-com.svg",
    "images/gear-svgrepo-com.svg",
    "images/touch-svgrepo-com.svg"};

static inline const bool LoadIconTexture(SDL_Renderer *renderer,
                                         std::string path,
                                         SDL_Texture *texture) {

  texture = IMG_LoadTexture(renderer, path.c_str());
  if (texture == NULL) {
    SDL_LogError(0,
                 "Unable to load image %s! "
                 "SDL Error: %s\n",
                 path.c_str(), SDL_GetError());
    return false;
  } else {
    return true;
  }
}

class Style {
private:
  SDL_Texture *iconTextures[NUM_ICONS];
  TTF_Font *font;
  SDL_Texture *particleTexture;

public:
  Style(SDL_Renderer *renderer) {

    font = TTF_OpenFont("fonts/Roboto-Medium.ttf", 36);
    if (font == NULL) {
      SDL_LogError(0, "failed to load font: %s\n", SDL_GetError());
    }
    for (auto &iconType : iconTypes) {
      iconTextures[static_cast<size_t>(iconType)] = NULL;
      auto path = iconPaths[static_cast<size_t>(iconType)];
      iconTextures[static_cast<size_t>(iconType)] =
          IMG_LoadTexture(renderer, path);

      if (iconTextures[static_cast<size_t>(iconType)] == NULL) {
        SDL_LogError(0,
                     "Unable to load image %s! "
                     "SDL Error: %s\n",
                     path, SDL_GetError());
      }

      SDL_Log("loaded %d", iconType);
    }
    auto particleTexturePath = "images/circle-waveform-lines-svgrepo-com.svg";
    particleTexture = IMG_LoadTexture(renderer, particleTexturePath);
    if (particleTexture == NULL) {
      SDL_LogError(0,
                   "Unable to load image %s! "
                   "SDL Error: %s\n",
                   particleTexturePath, SDL_GetError());
    }
  }
  ~Style() {
    TTF_CloseFont(font);
    font = NULL;
    for (auto &iconType : iconTypes) {
      SDL_DestroyTexture(iconTextures[static_cast<size_t>(iconType)]);
    }
    SDL_DestroyTexture(particleTexture);
  }
  SDL_Color color0 = SDL_Color{.r = 0xd6, .g = 0x02, .b = 0x70, .a = 0xff};
  SDL_Color color1 = SDL_Color{.r = 0x9b, .g = 0x4f, .b = 0x96, .a = 0xff};
  SDL_Color color2 = SDL_Color{.r = 0x00, .g = 0x38, .b = 0xa8, .a = 0xff};
  SDL_Color inactiveColor =
      SDL_Color{.r = 0x1b, .g = 0x1b, .b = 0x1b, .a = 0xff};
  SDL_Color inactiveIconColor =
      SDL_Color{.r = 0x6b, .g = 0x6b, .b = 0x6b, .a = 0xff};
  SDL_Color hoverColor = SDL_Color{.r = 0xa0, .g = 0xa0, .b = 0xa0, .a = 0xff};
  SDL_Color unavailableColor =
      SDL_Color{.r = 0x2b, .g = 0x2b, .b = 0x2b, .a = 0xff};
  SDL_Texture *getParticleTexture() const { return particleTexture; }
  inline SDL_Texture *getIconTexture(IconType type) const {
    if (iconTextures[static_cast<int>(type)] == NULL) {
      SDL_Log("%d its null!", type);
    }
    return iconTextures[static_cast<int>(type)];
  }

  inline TTF_Font *getFont() const { return font; }

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

  inline const SDL_Color getWidgetIconColor(const WidgetState state) const {

    switch (state) {
    case INACTIVE:
      return inactiveIconColor;
      break;
    case HOVER:
      return color1;
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
  Style(Style &) = delete;
  void operator=(Style &) = delete;
};
