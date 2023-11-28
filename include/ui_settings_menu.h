#pragma once

#include "ui_abstract.h"
#include "ui_navigation.h"
#include "widget_button.h"
struct SettingsMenu : public AbstractUI {
  Navigation *navigation;
  SettingsMenu(Navigation *_navigation) : navigation(_navigation) {}
  Button newGameButton;
  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto buttonWidth = shape.halfSize.x / 2;
    auto buttonHeight = 100.0;
    newGameButton = Button{
        .labelText = "new game",
        .shape = {.position = {.x = shape.halfSize.x, .y = shape.halfSize.y},
                  .halfSize = {.x = buttonWidth / 2,
                               .y = static_cast<float>(buttonHeight / 2.0)}}};
  };

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure){

  };

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure){

  };

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position, const float pressure){

  };

  virtual void handleMouseMove(const vec2f_t &mousePosition){

  };

  virtual void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoButtonClick(&newGameButton, mousePosition)) {
      navigation->page = Navigation::NEW_GAME;
    }
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition){

  };

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
    DrawButton(newGameButton, renderer, style);
  };
};
