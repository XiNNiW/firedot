#pragma once

#include "SDL_render.h"

#include "collider.h"
#include "metaphor.h"
#include "save_state.h"
#include "sequencer.h"
#include "ui_keyboard.h"
#include "ui_navigation.h"
#include "ui_play_instrument.h"
#include "ui_settings_menu.h"
#include "ui_sound_edit.h"
#include "widget_button.h"
#include "widget_radio_button.h"

struct UserInterface {
  Navigation navigation;
  PlayInstrumentUI playInstrumentUI;
  SettingsMenu settingsUI;
  AxisAlignedBoundingBox shape;

  UserInterface(Synthesizer<float> *synth, Sequencer *sequencer, Game *game,
                SaveState *saveState)
      : playInstrumentUI(
            PlayInstrumentUI(synth, sequencer, game, saveState, &navigation)),
        settingsUI(SettingsMenu(&navigation, saveState, synth)),
        navigation(this) {}

  inline void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    playInstrumentUI.buildLayout(shape);
    settingsUI.buildLayout(shape);
  }

  inline void refreshLayout() { buildLayout(shape); }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
    switch (navigation.getPage()) {
    case Navigation::NEW_GAME:
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
