#pragma once

#include "SDL_render.h"

#include "collider.h"
#include "metaphor.h"
#include "sequencer.h"
#include "ui_abstract.h"
#include "ui_keyboard.h"
#include "ui_mapping.h"
#include "ui_navigation.h"
#include "ui_play_instrument.h"
#include "ui_settings_menu.h"
#include "ui_sound_edit.h"
#include "widget_button.h"
#include "widget_radio_button.h"

enum IconType { MENU, SYNTH_SELECT };

struct ChooseCharacterUI {
  InstrumentMetaphorType *instrumentMetaphor;
  RadioGroup characterChooser;

  ChooseCharacterUI(InstrumentMetaphorType *_instrumentMetaphor)
      : instrumentMetaphor(_instrumentMetaphor) {}

  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {

    std::vector<std::string> instrumentOptionLabels = {};
    for (auto &instrumentType : InstrumentMetaphorTypes) {
      instrumentOptionLabels.push_back(
          InstrumentMetaphorTypeDisplayNames[instrumentType]);
    }
    characterChooser = RadioGroup::MakeRadioGroup(instrumentOptionLabels, 0);
    characterChooser.buildLayout(
        {.position = {.x = shape.halfSize.x, .y = shape.halfSize.y},
         .halfSize = {.x = shape.halfSize.x - 30, .y = 75}});
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
  Navigation *navigation = NULL;
  ChooseCharacterUI chooseCharacterUI;
  MappingUI mappingUI;
  SoundEditUI soundEditUI;
  Button previousButton;
  Button nextButton;
  float sideMargin = 15;
  float bottomMargin = 15;
  InstrumentSetupUI(Synthesizer<float> *synth,
                    SensorMapping<float> *sensorMapping,
                    InstrumentMetaphorType *instrumentMetaphor,
                    Navigation *_navigation)
      : chooseCharacterUI(ChooseCharacterUI(instrumentMetaphor)),
        mappingUI(MappingUI::MakeMappingUI(sensorMapping)),
        soundEditUI(SoundEditUI::MakeSoundEditUI(synth, sensorMapping)),
        navigation(_navigation) {}
  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto buttonHeight = 100;
    auto buttonWidth = 200;
    previousButton = Button{
        .labelText = "previous",
        .shape = {.position = {.x = static_cast<float>(0 + sideMargin +
                                                       buttonWidth / 2.0),
                               .y = static_cast<float>(
                                   height - buttonHeight / 2.0 - bottomMargin)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 2.0)}}};
    nextButton = Button{
        .labelText = "next",
        .shape = {.position = {.x = static_cast<float>(shape.halfSize.x * 2 -
                                                       buttonWidth / 2.0 -
                                                       sideMargin),
                               .y = static_cast<float>(
                                   height - buttonHeight / 2.0 - bottomMargin)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 2.0)}}};

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
    if (DoButtonClick(&nextButton, mousePosition)) {
      switch (page) {
      case CHARACTER_CHOOSER:
        page = MAPPING;
        break;
      case MAPPING:
        page = SOUND_EDIT;
        break;
      case SOUND_EDIT:
        page = CHARACTER_CHOOSER;
        navigation->page = Navigation::INSTRUMENT;
        break;
      }
    }
    if (DoButtonClick(&previousButton, mousePosition)) {
      switch (page) {
      case CHARACTER_CHOOSER:
        break;
      case MAPPING:
        page = CHARACTER_CHOOSER;
        break;
      case SOUND_EDIT:
        page = MAPPING;
        break;
      }
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
    SDL_SetRenderDrawColor(renderer, style.hoverColor.r, style.hoverColor.g,
                           style.hoverColor.b, style.hoverColor.a);
    SDL_RenderDrawLine(
        renderer, 15,
        previousButton.shape.position.y - previousButton.shape.halfSize.y - 10,
        nextButton.shape.position.x + nextButton.shape.halfSize.x,
        previousButton.shape.position.y - previousButton.shape.halfSize.y - 10);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    DrawButton(previousButton, renderer, style);
    DrawButton(nextButton, renderer, style);
  };
};

struct UserInterface : public AbstractUI {
  Navigation navigation;

  InstrumentSetupUI instrumentSetupUI;
  PlayInstrumentUI playInstrumentUI;
  SettingsMenu settingsUI;

  UserInterface(Synthesizer<float> *synth, SensorMapping<float> *sensorMapping,
                Sequencer *sequencer,
                InstrumentMetaphorType *instrumentMetaphor)
      : instrumentSetupUI(InstrumentSetupUI(synth, sensorMapping,
                                            instrumentMetaphor, &navigation)),
        playInstrumentUI(PlayInstrumentUI(synth, sequencer, sensorMapping,
                                          instrumentMetaphor, &navigation)),
        settingsUI(SettingsMenu(&navigation)) {}

  inline void buildLayout(const AxisAlignedBoundingBox &shape) {

    instrumentSetupUI.buildLayout(shape);
    playInstrumentUI.buildLayout(shape);
    settingsUI.buildLayout(shape);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.handleFingerMove(fingerId, position, pressure);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.handleFingerMove(fingerId, position, pressure);
      break;
    case Navigation::SETTINGS:
      settingsUI.handleFingerMove(fingerId, position, pressure);
      break;
    }
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.handleFingerDown(fingerId, position, pressure);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.handleFingerDown(fingerId, position, pressure);
      break;
    case Navigation::SETTINGS:
      settingsUI.handleFingerDown(fingerId, position, pressure);
      break;
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.handleFingerUp(fingerId, position, pressure);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.handleFingerUp(fingerId, position, pressure);
      break;
    case Navigation::SETTINGS:
      settingsUI.handleFingerUp(fingerId, position, pressure);
      break;
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.handleMouseMove(mousePosition);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.handleMouseMove(mousePosition);
      break;
    case Navigation::SETTINGS:
      settingsUI.handleMouseMove(mousePosition);
      break;
    }
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.handleMouseDown(mousePosition);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.handleMouseDown(mousePosition);
      break;
    case Navigation::SETTINGS:
      settingsUI.handleMouseDown(mousePosition);
      break;
    }
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.handleMouseUp(mousePosition);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.handleMouseUp(mousePosition);
      break;
    case Navigation::SETTINGS:
      settingsUI.handleMouseUp(mousePosition);
      break;
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    switch (navigation.page) {
    case Navigation::NEW_GAME:
      instrumentSetupUI.draw(renderer, style);
      break;
    case Navigation::INSTRUMENT:
      playInstrumentUI.draw(renderer, style);
      break;
    case Navigation::SETTINGS:
      settingsUI.draw(renderer, style);
      break;
    }
  }
};
