#pragma once
#include "collider.h"
#include "pitch_collection.h"
#include "save_state.h"
#include "synthesis.h"
#include "ui_keyboard.h"
#include "ui_load_savefile.h"
#include "ui_navigation.h"
#include "ui_option_popup.h"
#include "widget_button.h"
#include "widget_hslider.h"
#include "widget_state.h"
#include <vector>

struct SettingsMenu {
  Navigation *navigation;
  SaveState *saveState;
  Synthesizer<float> *synth;
  // FilebrowserUI filebrowser;
  bool showFileBrowser = false;
  AxisAlignedBoundingBox shape;
  SettingsMenu(Navigation *_navigation, SaveState *_saveState,
               Synthesizer<float> *_synth)
      : navigation(_navigation), saveState(_saveState), synth(_synth) {}
  HSlider keySlider;
  Button changeScaleButton;
  Button saveGameButton;
  Button loadGameButton;
  OptionPopupUI scaleSelectPopup;
  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    // filebrowser.buildLayout(shape);
    auto buttonWidth = shape.halfSize.x / 2;
    auto buttonHeight = 100.0;
    auto buttonMargin = 50;
    //  newGameButton =
    //      MakeButton("new game",
    //                 {.position = {.x = shape.halfSize.x, .y =
    //                 shape.halfSize.y},
    //                  .halfSize = {.x = buttonWidth / 2,
    //                               .y = static_cast<float>(buttonHeight
    //     / 2.0)}});
    keySlider = MakeHSlider(
        GetNoteName(saveState->sensorMapping.key),
        {.position =
             {
                 .x = shape.position.x,
                 .y = static_cast<float>(shape.position.y - buttonHeight * 2 -
                                         buttonMargin * 3),
             },
         .halfSize = {.x = shape.halfSize.x - 2 * buttonMargin,
                      .y = static_cast<float>(buttonHeight / 2.0)}});
    changeScaleButton =
        MakeButton("change scale",
                   {.position = {.x = shape.halfSize.x, .y = shape.halfSize.y},
                    .halfSize = {.x = buttonWidth / 2,
                                 .y = static_cast<float>(buttonHeight / 2.0)}});
    saveGameButton = MakeButton(
        "save game",
        {.position = {.x = shape.halfSize.x,
                      .y = static_cast<float>(buttonHeight + buttonMargin +
                                              shape.halfSize.y)},
         .halfSize = {.x = buttonWidth / 2,
                      .y = static_cast<float>(+buttonHeight / 2.0)}});
    loadGameButton =
        MakeButton("load game",
                   {.position = {.x = shape.halfSize.x,
                                 .y = static_cast<float>(
                                     (buttonHeight + buttonMargin) * 2 +
                                     shape.halfSize.y)},
                    .halfSize = {.x = buttonWidth / 2,
                                 .y = static_cast<float>(buttonHeight / 2.0)}});

    scaleSelectPopup.buildLayout(shape);
  };

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure){

  };

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure){

  };

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position, const float pressure){};

  virtual void handleMouseMove(const vec2f_t &mousePosition) {
    float key = 0;
    if (DoHSliderDrag(&keySlider, &key, mousePosition)) {
      saveState->sensorMapping.key = floor(key * 12);
      keySlider.label.setText(GetNoteName(saveState->sensorMapping.key));
    }
  };

  virtual void handleMouseDown(const vec2f_t &mousePosition) {

    //  switch (filebrowser.mode) {

    // case FilebrowserUI::CLOSED:

    // if (DoButtonClick(&newGameButton, mousePosition)) {
    //   navigation->setPage(Navigation::NEW_GAME);
    // }

    if (scaleSelectPopup.isOpen()) {
      int selection;
      if (scaleSelectPopup.doClick(mousePosition, &selection)) {
        saveState->sensorMapping.scaleType = static_cast<ScaleType>(selection);
      }
    } else {

      float key = 0;
      if (DoHSliderClick(&keySlider, &key, mousePosition)) {
        saveState->sensorMapping.key = floor(key * 12);
        keySlider.label.setText(GetNoteName(saveState->sensorMapping.key));
      }

      if (DoButtonClick(&changeScaleButton, mousePosition)) {
        std::vector<std::string> scaleNames = std::vector<std::string>();
        for (auto type : ScaleTypes) {
          scaleNames.push_back(getDisplayName(type));
        }
        scaleSelectPopup.open(
            scaleNames,
            static_cast<size_t>(saveState->sensorMapping.scaleType));
      }
      if (DoButtonClick(&saveGameButton, mousePosition)) {
        SaveState::SaveGame("game name", *synth, saveState);
      }
      if (DoButtonClick(&loadGameButton, mousePosition)) {
        SaveState::LoadGame("game name", synth, saveState);
        // filebrowser.open();
      }
    }

    //    break;
    // case FilebrowserUI::OPEN:
    //   filebrowser.handleMouseDown(mousePosition);
    //  break;
    // case FilebrowserUI::SELECTED:
    //  break;
    // }
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition) {

    //  switch (filebrowser.mode) {
    //  case FilebrowserUI::CLOSED:
    //    break;
    //  case FilebrowserUI::OPEN:
    //    break;
    //  case FilebrowserUI::SELECTED:
    //    SaveState::LoadGame(filebrowser.selectedPath, synth, saveState);
    //    filebrowser.close();
    //    break;
    //  }

    keySlider.state = INACTIVE;
  };

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
    //  DrawButton(&newGameButton, renderer, style);

    if (scaleSelectPopup.isOpen()) {
      scaleSelectPopup.draw(renderer, style);
    } else {
      DrawHSlider(&keySlider, saveState->sensorMapping.key / 12.0, renderer,
                  style);
      DrawButton(&changeScaleButton, renderer, style);
      DrawButton(&saveGameButton, renderer, style);
      DrawButton(&loadGameButton, renderer, style);
    }

    // filebrowser.draw(renderer, style);
  };
};
