#pragma once

#include "widget.h"

struct Navigation {
  enum Page { KEYBOARD, MAPPING, SOUND_EDIT } page = KEYBOARD;
};
static const int NUM_NAVIGATION_PAGES = 3;
static_assert((NUM_NAVIGATION_PAGES - 1) == Navigation::SOUND_EDIT,
              "Navigation enum size does not match NavigationPages");
static const Navigation::Page NavigationPages[NUM_NAVIGATION_PAGES] = {
    Navigation::KEYBOARD, Navigation::MAPPING, Navigation::SOUND_EDIT};

static const char *NavigationPageDisplayNames[NUM_NAVIGATION_PAGES] = {
    "keyboard",
    "sensor mapping",
    "sound edit",
};

struct NavigationUI {
  RadioGroup pages;
  Navigation *navigation = NULL;
  float topMargin = 5;
  float pageMargin = 25;
  float bottomMargin = 15;
  float seperatorHeight = 2;
  float buttonHeight = 200;
  AxisAlignedBoundingBox shape;
  static inline const NavigationUI MakeNavigationUI(Navigation *navigation) {

    const size_t initialSynthTypeSelection = navigation->page;
    std::vector<std::string> labels;
    for (auto &page : NavigationPages) {
      labels.push_back(NavigationPageDisplayNames[page]);
    }

    return NavigationUI{
        .pages = RadioGroup::MakeRadioGroup(labels, initialSynthTypeSelection),
        .navigation = navigation};
    ;
  }
  inline void buildLayout(const float width, const float height) {

    buttonHeight = height / 18.0;
    pages.buildLayout(
        {.position = {.x = static_cast<float>(width / 2.0),
                      .y = static_cast<float>((buttonHeight / 2) + topMargin)},
         .halfSize = {.x = static_cast<float>((width - pageMargin) / 2.0),
                      .y = static_cast<float>(buttonHeight / 2.0)}});
    shape = {.position = {0, 0},
             .halfSize = {width / 2, (topMargin + bottomMargin + buttonHeight +
                                      seperatorHeight) /
                                         2}};
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}

  inline void handleMouseMove(const vec2f_t &mousePosition) {}

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoClickRadioGroup(&pages, mousePosition)) {

      navigation->page = NavigationPages[pages.selectedIndex];
    };
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {}

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    DrawRadioGroup(pages, renderer, style);
    auto seperatorRect = SDL_Rect{
        .x = static_cast<int>(pageMargin),
        .y = static_cast<int>(buttonHeight + topMargin + bottomMargin / 2),
        .w = static_cast<int>(pages.shape.halfSize.x * 2 - pageMargin),
        .h = static_cast<int>(seperatorHeight)};
    SDL_SetRenderDrawColor(renderer, style.inactiveColor.r,
                           style.inactiveColor.g, style.inactiveColor.b,
                           style.inactiveColor.a);
    SDL_RenderFillRect(renderer, &seperatorRect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  }
};
