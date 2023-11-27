#pragma once

#include "SDL_render.h"

#include "collider.h"
#include "metaphor.h"
#include "ui_abstract.h"
#include "ui_keyboard.h"
#include "ui_mapping.h"
#include "ui_navigation.h"
#include "ui_sound_edit.h"
#include "widget_button.h"
#include "widget_radio_button.h"

enum IconType { MENU, SYNTH_SELECT };

struct ChooseCharacterUI {
  InstrumentMetaphorType *instrumentMetaphor;
  RadioGroup characterChooser;

  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {

    std::vector<std::string> instrumentOptionLabels = {};
    for (auto &instrumentType : InstrumentMetaphorTypes) {
      instrumentOptionLabels.push_back(SynthTypeDisplayNames[instrumentType]);
    }
    characterChooser = RadioGroup::MakeRadioGroup(instrumentOptionLabels, 0);
    characterChooser.shape = {
        .position = {.x = 15, .y = shape.halfSize.y},
        .halfSize = {.x = shape.halfSize.x - 30, .y = 75}};
  };

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position,
                                const float pressure){};

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position,
                                const float pressure){};

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position, const float pressure){};

  virtual void handleMouseMove(const vec2f_t &mousePosition){};

  virtual void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoClickRadioGroup(&characterChooser, mousePosition)) {
      *instrumentMetaphor =
          InstrumentMetaphorTypes[characterChooser.selectedIndex];
    }
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition){};

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
    DrawRadioGroup(characterChooser, renderer, style);
  };
};

struct InstrumentSetupUI : public AbstractUI {

  enum Page { CHARACTER_CHOOSER, MAPPING, SOUND_EDIT } page = CHARACTER_CHOOSER;
  ;
  static const int NUM_NAVIGATION_PAGES = 3;
  static_assert((NUM_NAVIGATION_PAGES - 1) == Page::SOUND_EDIT,
                "Navigation enum size does not match NavigationPages");
  constexpr static const Page NavigationPages[NUM_NAVIGATION_PAGES] = {
      Page::CHARACTER_CHOOSER, Page::MAPPING, Page::SOUND_EDIT};
  ChooseCharacterUI chooseCharacterUI;
  MappingUI mappingUI;
  SoundEditUI soundEditUI;
  Button previousButton;
  Button nextButton;
  float sideMargin = 15;
  float bottomMargin = 15;
  InstrumentSetupUI(Synthesizer<float> *synth,
                    SensorMapping<float> *sensorMapping) {

    mappingUI = MappingUI::MakeMappingUI(sensorMapping);
    soundEditUI = SoundEditUI::MakeSoundEditUI(synth, sensorMapping);
  }
  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto buttonHeight = 100;
    auto buttonWidth = 200;
    previousButton = Button{
        .shape = {.position = {.x = 0 + sideMargin,
                               .y = static_cast<float>(
                                   height - buttonHeight / 2.0 + bottomMargin)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 2.0)}}};
    nextButton = Button{
        .shape = {.position = {.x = static_cast<float>(buttonWidth / 2.0 +
                                                       sideMargin),
                               .y = static_cast<float>(
                                   height - buttonHeight / 2.0 + bottomMargin)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 2.0)}}};

    AxisAlignedBoundingBox topShape = AxisAlignedBoundingBox{
        .position = {.x = 0, .y = 0},
        .halfSize = {.x = shape.halfSize.x,
                     .y = (shape.halfSize.y * 2 - buttonHeight - bottomMargin) /
                          2}};
    chooseCharacterUI.buildLayout(topShape);
    mappingUI.buildLayout(topShape);
    soundEditUI.buildLayout(topShape);
  };

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure) {
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

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure) {
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

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position, const float pressure) {
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

  virtual void handleMouseMove(const vec2f_t &mousePosition) {
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

  virtual void handleMouseDown(const vec2f_t &mousePosition) {
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
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition) {
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

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
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
  };
};

struct UserInterface : public AbstractUI {
  Navigation navigation;

  NavigationUI navigationUI;
  KeyboardUI keyboardUI;
  MappingUI mappingUI;
  SoundEditUI soundEditUI;
  UserInterface(Synthesizer<float> *synth, SensorMapping<float> *sensorMapping)
      : keyboardUI(KeyboardUI(synth)),
        navigationUI(NavigationUI::MakeNavigationUI(&navigation)),
        mappingUI(MappingUI::MakeMappingUI(sensorMapping)),
        soundEditUI(SoundEditUI::MakeSoundEditUI(synth, sensorMapping)) {}

  inline void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto navUIShape = AxisAlignedBoundingBox{
        .position = {.x = 0, .y = 0},
        .halfSize = {.x = width / 2, .y = 30},
    };

    navigationUI.buildLayout(navUIShape);
    auto lowerScreen = AxisAlignedBoundingBox{
        .position = {.x = 0, .y = navigationUI.shape.halfSize.y * 2},
        .halfSize = {.x = width / 2,
                     .y = (height - navigationUI.shape.halfSize.y * 2) / 2}};
    mappingUI.buildLayout(lowerScreen);
    keyboardUI.buildLayout(lowerScreen);
    soundEditUI.buildLayout(lowerScreen);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {

    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.handleFingerMove(fingerId, position, pressure);
      break;
    case Navigation::MAPPING:
      mappingUI.handleFingerMove(fingerId, position, pressure);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleFingerMove(fingerId, position, pressure);
      break;
    }
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {

    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.handleFingerDown(fingerId, position, pressure);
      break;
    case Navigation::MAPPING:
      mappingUI.handleFingerDown(fingerId, position, pressure);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleFingerDown(fingerId, position, pressure);
      break;
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.handleFingerUp(fingerId, position, pressure);
      break;
    case Navigation::MAPPING:
      mappingUI.handleFingerUp(fingerId, position, pressure);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleFingerUp(fingerId, position, pressure);
      break;
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.handleMouseMove(mousePosition);
      break;
    case Navigation::MAPPING:
      mappingUI.handleMouseMove(mousePosition);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleMouseMove(mousePosition);
      break;
    }
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    navigationUI.handleMouseDown(mousePosition);
    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.handleMouseDown(mousePosition);
      break;
    case Navigation::MAPPING:
      mappingUI.handleMouseDown(mousePosition);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleMouseDown(mousePosition);
      break;
    }
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.handleMouseUp(mousePosition);
      break;
    case Navigation::MAPPING:
      mappingUI.handleMouseUp(mousePosition);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleMouseUp(mousePosition);
      break;
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    switch (navigation.page) {
    case Navigation::INSTRUMENT:
      keyboardUI.draw(renderer, style);
      break;
    case Navigation::MAPPING:
      mappingUI.draw(renderer, style);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.draw(renderer, style);
      break;
    }
    navigationUI.draw(renderer, style);
  }
};
