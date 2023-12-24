#pragma once

#include "save_state.h"
#include "synthesis.h"
#include "ui_abstract.h"
#include "ui_navigation.h"
#include "widget_button.h"
struct SettingsMenu : public AbstractUI {
  Navigation *navigation;
  SaveState *saveState;
  Synthesizer<float> *synth;
  SettingsMenu(Navigation *_navigation, SaveState *_saveState,
               Synthesizer<float> *_synth)
      : navigation(_navigation), saveState(_saveState), synth(_synth) {}
  Button newGameButton;
  Button saveGameButton;
  Button loadGameButton;
  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto buttonWidth = shape.halfSize.x / 2;
    auto buttonHeight = 100.0;
    auto buttonMargin = 50;
    newGameButton = Button{
        .label = Label("new game"),
        .shape = {.position = {.x = shape.halfSize.x, .y = shape.halfSize.y},
                  .halfSize = {.x = buttonWidth / 2,
                               .y = static_cast<float>(buttonHeight / 2.0)}}};
    saveGameButton = Button{
        .label = Label("save game"),
        .shape = {
            .position = {.x = shape.halfSize.x,
                         .y = static_cast<float>(buttonHeight + buttonMargin +
                                                 shape.halfSize.y)},
            .halfSize = {.x = buttonWidth / 2,
                         .y = static_cast<float>(+buttonHeight / 2.0)}}};
    loadGameButton = Button{
        .label = Label("load game"),
        .shape = {.position = {.x = shape.halfSize.x,
                               .y = static_cast<float>(
                                   (buttonHeight + buttonMargin) * 2 +
                                   shape.halfSize.y)},
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
    if (DoButtonClick(&saveGameButton, mousePosition)) {
      SaveGame("game name", *synth, *saveState);
    }
    if (DoButtonClick(&loadGameButton, mousePosition)) {
      LoadGame("game name", synth, saveState);
    }
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition){

  };

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
    DrawButton(&newGameButton, renderer, style);
    DrawButton(&saveGameButton, renderer, style);
    DrawButton(&loadGameButton, renderer, style);
  };
};
