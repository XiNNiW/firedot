#pragma once

#include "SDL_render.h"

#include "ui_keyboard.h"
#include "ui_mapping.h"
#include "ui_navigation.h"
#include "ui_sound_edit.h"

enum IconType { MENU, SYNTH_SELECT };

struct UserInterface {
  Navigation navigation;

  NavigationUI navigationUI;
  KeyboardUI keyboardUI;
  MappingUI mappingUI;
  SoundEditUI soundEditUI;
  UserInterface(Synthesizer<float> *synth,
                SensorMapping<float> *sensorMapping) {

    navigationUI = NavigationUI::MakeNavigationUI(&navigation);
    keyboardUI = KeyboardUI::MakeKeyboardUI(&navigationUI, synth);
    mappingUI = MappingUI::MakeMappingUI(&navigationUI, sensorMapping);
    soundEditUI =
        SoundEditUI::MakeSoundEditUI(&navigationUI, synth, sensorMapping);
  }

  inline void buildLayout(const float width, const float height) {

    navigationUI.buildLayout(width, height);
    mappingUI.buildLayout(width, height);
    keyboardUI.buildLayout(width, height);
    soundEditUI.buildLayout(width, height);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {

    switch (navigation.page) {
    case Navigation::KEYBOARD:
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
    case Navigation::KEYBOARD:
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
    case Navigation::KEYBOARD:
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
    case Navigation::KEYBOARD:
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
    switch (navigation.page) {
    case Navigation::KEYBOARD:
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
    case Navigation::KEYBOARD:
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
    case Navigation::KEYBOARD:
      keyboardUI.draw(renderer, style);
      break;
    case Navigation::MAPPING:
      mappingUI.draw(renderer, style);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.draw(renderer, style);
      break;
    }
  }
};
