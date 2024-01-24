#pragma once

#include "metaphor.h"
#include "save_state.h"
#include "ui_choose_character.h"
#include "ui_mapping.h"
#include "ui_navigation.h"
#include "ui_sound_edit.h"
#include "widget_state.h"
#include "widget_style.h"

struct InstrumentSetupUI {

  enum Page { CHARACTER_CHOOSER, MAPPING, SOUND_EDIT } page = CHARACTER_CHOOSER;

  static const int NUM_NAVIGATION_PAGES = 3;
  static_assert((NUM_NAVIGATION_PAGES - 1) == Page::SOUND_EDIT,
                "Navigation enum size does not match NavigationPages");
  constexpr static const Page NavigationPages[NUM_NAVIGATION_PAGES] = {
      Page::CHARACTER_CHOOSER, Page::MAPPING, Page::SOUND_EDIT};
  Navigation *navigation = NULL;
  ChooseCharacterUI chooseCharacterUI;
  MappingUI mappingUI;
  SoundEditUI soundEditUI;
  Button previousButton;
  Button nextButton;
  float sideMargin = 15;
  float bottomMargin = 15;
  InstrumentSetupUI(Synthesizer<float> *synth, SaveState *saveState,
                    Navigation *_navigation)
      : chooseCharacterUI(ChooseCharacterUI(saveState)),
        mappingUI(MappingUI(saveState)),
        soundEditUI(SoundEditUI(synth, &saveState->sensorMapping, saveState)),
        navigation(_navigation) {}
  void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto buttonHeight = 100;
    auto buttonWidth = 200;
    previousButton = Button{
        .label = Label("previous"),
        .shape = {.position = {.x = static_cast<float>(0 + sideMargin +
                                                       buttonWidth / 2.0),
                               .y = static_cast<float>(
                                   height - buttonHeight / 2.0 - bottomMargin)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 2.0)}},
        .iconType = IconType::LEFT_ARROW};
    nextButton = Button{
        .label = Label("next"),
        .shape = {.position = {.x = static_cast<float>(shape.halfSize.x * 2 -
                                                       buttonWidth / 2.0 -
                                                       sideMargin),
                               .y = static_cast<float>(
                                   height - buttonHeight / 2.0 - bottomMargin)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 2.0)}},
        .iconType = IconType::RIGHT_ARROW};

    AxisAlignedBoundingBox topShape = AxisAlignedBoundingBox{
        .position = {.x = shape.halfSize.x,
                     .y = (shape.halfSize.y * 2 - buttonHeight - bottomMargin) /
                          2},
        .halfSize = {.x = shape.halfSize.x,
                     .y = (shape.halfSize.y * 2 - buttonHeight - bottomMargin) /
                          2}};
    chooseCharacterUI.buildLayout(topShape);
    mappingUI.buildLayout(topShape);
    soundEditUI.buildLayout(topShape);
    previousButton.state = HIDDEN;
  };

  void handleFingerMove(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.handleFingerMove(fingerId, position, pressure);
      break;
    case MAPPING:
      mappingUI.handleFingerMove(fingerId, position, pressure);
      break;
    case SOUND_EDIT:
      soundEditUI.handleFingerMove(fingerId, position, pressure);
      break;
    }
  };

  void handleFingerDown(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.handleFingerDown(fingerId, position, pressure);
      break;
    case MAPPING:
      mappingUI.handleFingerDown(fingerId, position, pressure);
      break;
    case SOUND_EDIT:
      soundEditUI.handleFingerDown(fingerId, position, pressure);
      break;
    }
  };

  void handleFingerUp(const SDL_FingerID &fingerId, const vec2f_t &position,
                      const float pressure) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.handleFingerUp(fingerId, position, pressure);
      break;
    case MAPPING:
      mappingUI.handleFingerUp(fingerId, position, pressure);
      break;
    case SOUND_EDIT:
      soundEditUI.handleFingerUp(fingerId, position, pressure);
      break;
    }
  };

  void handleMouseMove(const vec2f_t &mousePosition) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.handleMouseMove(mousePosition);
      break;
    case MAPPING:
      mappingUI.handleMouseMove(mousePosition);
      break;
    case SOUND_EDIT:
      soundEditUI.handleMouseMove(mousePosition);
      break;
    }
  };

  void handleMouseDown(const vec2f_t &mousePosition) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.handleMouseDown(mousePosition);
      break;
    case MAPPING:
      mappingUI.handleMouseDown(mousePosition);
      break;
    case SOUND_EDIT:
      soundEditUI.handleMouseDown(mousePosition);
      break;
    }
    if (DoButtonClick(&nextButton, mousePosition)) {
      switch (page) {
      case CHARACTER_CHOOSER:
        previousButton.state = INACTIVE;
        page = MAPPING;
        mappingUI.refreshLayout();
        break;
      case MAPPING:
        page = SOUND_EDIT;
        break;
      case SOUND_EDIT:
        page = CHARACTER_CHOOSER;
        navigation->setPage(Navigation::INSTRUMENT);
        break;
      }
    }
    if (DoButtonClick(&previousButton, mousePosition)) {
      switch (page) {
      case CHARACTER_CHOOSER:
        break;
      case MAPPING:
        page = CHARACTER_CHOOSER;
        previousButton.state = HIDDEN;
        break;
      case SOUND_EDIT:
        page = MAPPING;
        break;
      }
    }
  };

  void handleMouseUp(const vec2f_t &mousePosition) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.handleMouseUp(mousePosition);
      break;
    case MAPPING:
      mappingUI.handleMouseUp(mousePosition);
      break;
    case SOUND_EDIT:
      soundEditUI.handleMouseUp(mousePosition);
      break;
    }
  };

  void draw(SDL_Renderer *renderer, const Style &style) {
    switch (page) {
    case CHARACTER_CHOOSER:
      chooseCharacterUI.draw(renderer, style);
      break;
    case MAPPING:
      mappingUI.draw(renderer, style);
      break;
    case SOUND_EDIT:
      soundEditUI.draw(renderer, style);
      break;
    }
    DrawLine(
        {.x = previousButton.shape.position.x - previousButton.shape.halfSize.x,
         .y = previousButton.shape.position.y -
              previousButton.shape.halfSize.y - 10},
        {.x = nextButton.shape.position.x + nextButton.shape.halfSize.x,
         .y = previousButton.shape.position.y -
              previousButton.shape.halfSize.y - 10},
        renderer, style.hoverColor);
    DrawButton(&previousButton, renderer, style);
    DrawButton(&nextButton, renderer, style);
  };
};
