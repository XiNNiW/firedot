#pragma once

#include "SDL_render.h"
#include "metaphor.h"
#include "save_state.h"
#include "ui_abstract.h"
#include "ui_keyboard.h"
#include "ui_mapping.h"
#include "ui_navigation.h"
#include "ui_sequencer.h"
#include "ui_settings_menu.h"
#include "ui_sound_edit.h"
#include "ui_touch_pad.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_radio_button.h"

struct PlayInstrumentUI {
  SaveState *saveState;
  Navigation *navigation;
  enum Page { PLAY, EDIT_SOUND, EDIT_MAPPING, SETTINGS } page = PLAY;
  static const int NUM_PAGES = 4;
  static_assert((NUM_PAGES - 1) == SETTINGS,
                "Navigation enum size does not match NavigationPages");
  constexpr static const Page Pages[NUM_PAGES] = {PLAY, EDIT_SOUND,
                                                  EDIT_MAPPING, SETTINGS};

  RadioGroup pageSelector;
  KeyboardUI keyboardUI;
  SequencerUI sequencerUI;
  TouchPadUI touchPadUI;
  SoundEditUI soundEditUI;
  MappingUI mappingUI;
  SettingsMenu settingsMenu;
  float topMargin = 15;
  float sideMargin = 15;

  PlayInstrumentUI(Synthesizer<float> *synth, Sequencer *sequencer,
                   InputMapping<float> *mapping, SaveState *_saveState,
                   Navigation *_navigation)
      : keyboardUI(KeyboardUI(synth, mapping)),
        sequencerUI(SequencerUI(sequencer)), touchPadUI(synth, mapping),
        soundEditUI(SoundEditUI::MakeSoundEditUI(synth, mapping)),
        mappingUI(MappingUI::MakeMappingUI(mapping, saveState)),
        settingsMenu(SettingsMenu(_navigation)), saveState(_saveState),
        navigation(_navigation) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto navGroupHeight = 100;
    auto buttonWidth = 200;

    pageSelector = RadioGroup::MakeRadioGroup(
        {"play", "edit sound", "edit sensors", "settings"}, PLAY);
    auto upperShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = static_cast<float>(navGroupHeight / 2.0)},
        .halfSize = {.x = shape.halfSize.x,
                     .y = static_cast<float>(navGroupHeight / 2.0)}};
    pageSelector.buildLayout(upperShape);

    auto lowerShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = navGroupHeight + topMargin + shape.position.y},
        .halfSize = {.x = shape.halfSize.x,
                     .y = static_cast<float>(
                         (shape.halfSize.y * 2 - navGroupHeight) / 2.0)}};
    keyboardUI.buildLayout(lowerShape);
    sequencerUI.buildLayout(lowerShape);
    soundEditUI.buildLayout(lowerShape);
    mappingUI.buildLayout(lowerShape);
    settingsMenu.buildLayout(lowerShape);
  };

  void handleFingerMove(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure) {
    switch (page) {

    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.handleFingerMove(fingerId, position, pressure);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.handleFingerMove(fingerId, position, pressure);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.handleFingerMove(fingerId, position, pressure);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.handleFingerMove(fingerId, position, pressure);
      break;
    case SETTINGS:
      settingsMenu.handleFingerMove(fingerId, position, pressure);
      break;
    case EDIT_MAPPING:
      mappingUI.handleFingerMove(fingerId, position, pressure);
      break;
    }
  };

  void handleFingerDown(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure) {
    switch (page) {

    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.handleFingerDown(fingerId, position, pressure);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.handleFingerDown(fingerId, position, pressure);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.handleFingerDown(fingerId, position, pressure);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.handleFingerDown(fingerId, position, pressure);
      break;
    case SETTINGS:
      settingsMenu.handleFingerDown(fingerId, position, pressure);
      break;
    case EDIT_MAPPING:
      mappingUI.handleFingerDown(fingerId, position, pressure);
      break;
    }
  };

  void handleFingerUp(const SDL_FingerID &fingerId, const vec2f_t &position,
                      const float pressure) {
    switch (page) {

    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.handleFingerUp(fingerId, position, pressure);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.handleFingerUp(fingerId, position, pressure);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.handleFingerUp(fingerId, position, pressure);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.handleFingerUp(fingerId, position, pressure);
      break;
    case SETTINGS:
      settingsMenu.handleFingerUp(fingerId, position, pressure);
      break;
    case EDIT_MAPPING:
      mappingUI.handleFingerUp(fingerId, position, pressure);
      break;
    }
  };

  void handleMouseMove(const vec2f_t &mousePosition) {

    switch (page) {

    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.handleMouseMove(mousePosition);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.handleMouseMove(mousePosition);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.handleMouseMove(mousePosition);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.handleMouseMove(mousePosition);
      break;
    case SETTINGS:
      settingsMenu.handleMouseMove(mousePosition);
      break;
    case EDIT_MAPPING:
      mappingUI.handleMouseMove(mousePosition);
      break;
    }
  };

  void handleMouseDown(const vec2f_t &mousePosition) {
    switch (page) {
    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.handleMouseDown(mousePosition);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.handleMouseDown(mousePosition);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.handleMouseDown(mousePosition);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.handleMouseDown(mousePosition);
      break;
    case SETTINGS:
      settingsMenu.handleMouseDown(mousePosition);
      break;
    case EDIT_MAPPING:
      mappingUI.handleMouseDown(mousePosition);
      break;
    }
    int selectedIndex = 0;
    if (DoClickRadioGroup(&pageSelector, mousePosition)) {
      page = Pages[pageSelector.selectedIndex];
    }
  };

  void handleMouseUp(const vec2f_t &mousePosition) {
    switch (page) {

    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.handleMouseUp(mousePosition);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.handleMouseUp(mousePosition);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.handleMouseUp(mousePosition);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.handleMouseUp(mousePosition);
      break;
    case SETTINGS:
      settingsMenu.handleMouseUp(mousePosition);
      break;
    case EDIT_MAPPING:
      mappingUI.handleMouseUp(mousePosition);
      break;
    }
  };

  void draw(SDL_Renderer *renderer, const Style &style) {

    switch (page) {

    case PLAY:
      switch (saveState->instrumentMetaphor) {
      case InstrumentMetaphorType::KEYBOARD:
        keyboardUI.draw(renderer, style);
        break;
      case InstrumentMetaphorType::SEQUENCER:
        sequencerUI.draw(renderer, style);
        break;
      case InstrumentMetaphorType::TOUCH_PAD:
        touchPadUI.draw(renderer, style);
      }
      break;
    case EDIT_SOUND:
      soundEditUI.draw(renderer, style);
      break;
    case SETTINGS:
      settingsMenu.draw(renderer, style);
      break;
    case EDIT_MAPPING:
      mappingUI.draw(renderer, style);
      break;
    }

    DrawRadioGroup(pageSelector, renderer, style);
    DrawLine({.x = pageSelector.shape.position.x -
                   pageSelector.shape.halfSize.x + 15,
              .y = pageSelector.shape.halfSize.y * 2 + 10},
             {.x = pageSelector.shape.position.x +
                   pageSelector.shape.halfSize.x - 15,
              .y = pageSelector.shape.halfSize.y * 2 + 10},
             renderer, style.hoverColor);
  };
};
