#pragma once

#include "collider.h"
#include "metaphor.h"
#include "save_state.h"
#include "sensor.h"
#include "sequencer.h"
#include "synthesis.h"
#include "ui_navigation.h"
#include "widget_state.h"
#include "widget_vslider.h"
#include <map>
#include <string>

inline const std::string GetNoteName(int note) {
  note = note % 12;
  switch (note) {
  case 0:
    return "c";
    break;
  case 1:
    return "c#";
    break;
  case 2:
    return "d";
    break;
  case 3:
    return "d#";
    break;
  case 4:
    return "e";
    break;
  case 5:
    return "f";
    break;
  case 6:
    return "f#";
    break;
  case 7:
    return "g";
    break;
  case 8:
    return "g#";
    break;
  case 9:
    return "a";
    break;
  case 10:
    return "a#";
    break;
  case 11:
    return "b";
    break;
  }
  return "";
}

struct KeyboardUI {

  static constexpr size_t SYNTH_SELECTED_RADIO_GROUP_SIZE = 3;
  static constexpr size_t NUM_KEY_BUTTONS = NoteMap::NUM_NOTES;

  Synthesizer<float> *synth = NULL;
  SaveState *saveState;

  std::map<SDL_FingerID, int> heldKeys;
  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  Button keyButtons[NUM_KEY_BUTTONS];

  float synthSelectWidth = 150;
  float synthSelectHeight = 50;
  float titleBarHeight = 100;
  float buttonMargin = 5;
  float topMargin = 15;

  KeyboardUI(Synthesizer<float> *_synth, SaveState *_saveState)
      : synth(_synth), saveState(_saveState) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto pageMargin = 50;
    auto radiobuttonMargin = 10;
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;

    synthSelectWidth = width / 6;
    synthSelectHeight = width / 8.0;

    titleBarHeight = yOffset + topMargin;

    auto keySize = width / 5.5;
    auto keyboardStartPositionX = 100;
    float keysPerRow = 5;

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {

      keyButtons[i] = Button{
          .label = Label(GetNoteName(synth->noteMap.notes[i])),
          .shape = AxisAlignedBoundingBox{
              .position =
                  vec2f_t{
                      .x = static_cast<float>(
                          shape.position.x - (keysPerRow / 2.0) * keySize +
                          keySize / 2 + (i % int(keysPerRow)) * keySize),
                      .y = static_cast<float>(titleBarHeight + keySize / 2.0 +
                                              floor(i / keysPerRow) * keySize)},
              .halfSize = vec2f_t{.x = static_cast<float>(keySize / 2),
                                  .y = static_cast<float>(keySize / 2)}}};
    }
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (DoButtonHover(&keyButtons[i], position)) {
        auto bentNote = synth->noteMap.notes[heldKeys[fingerId]];
        auto bendDestination = synth->noteMap.notes[i];
        // synth->bendNote(bentNote, bendDestination);
        saveState->sensorMapping.emitEvent(
            synth, KEYBOARD_KEY, float(i) / float(NUM_KEY_BUTTONS - 1));
        fingerPositions[fingerId] = i;
      }
    }

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      for (size_t j = 0; j < 10; ++j) {
        if ((fingerPositions[j] == i) && (keyButtons[i].state != ACTIVE)) {
          keyButtons[i].state = HOVER;
        }
      }
    }
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      // evaluate clicks
      if (DoButtonClick(&keyButtons[i], position, WidgetState::ACTIVE)) {
        // play sound
        // auto note = saveState->sensorMapping.notes[i];
        heldKeys[fingerId] = i;
        // saveState->sensorMapping.noteOn(synth, InputType::KEYBOARD_KEY,
        // mtof(note));
        saveState->sensorMapping.emitEvent(
            synth, ContinuousInputType::KEYBOARD_KEY,
            float(i) / float(NUM_KEY_BUTTONS - 1));
        // saveState->sensorMapping.emitEvent(synth,
        // ContinuousInputType::KEYBOARD_KEY,
        //                    mtof(note));
        saveState->sensorMapping.emitEvent(
            synth, MomentaryInputType::KEYBOARD_GATE, 1);
      }
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {

    if (auto buttonIdx = heldKeys[fingerId]) {
      keyButtons[buttonIdx].state = WidgetState::INACTIVE;
      saveState->sensorMapping.emitEvent(
          synth, ContinuousInputType::KEYBOARD_KEY,
          float(buttonIdx) / float(NUM_KEY_BUTTONS - 1));
      saveState->sensorMapping.emitEvent(synth,
                                         MomentaryInputType::KEYBOARD_GATE, 0);
      heldKeys.erase(fingerId);
    }

    if (fingerPositions[fingerId] > -1) {
      keyButtons[fingerPositions[fingerId]].state = INACTIVE;
    }

    if (heldKeys.size() == 0) {
      for (auto &button : keyButtons) {
        button.state = WidgetState::INACTIVE;
      }
    }

    fingerPositions[fingerId] = -1;
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {}

  inline void handleMouseDown(const vec2f_t &mousePosition) {}

  inline void handleMouseUp(const vec2f_t &mousePosition) {

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (keyButtons[i].state == WidgetState::ACTIVE) {
        keyButtons[i].state = WidgetState::INACTIVE;
        saveState->sensorMapping.emitEvent(
            synth, ContinuousInputType::KEYBOARD_KEY,
            float(i) / float(NUM_KEY_BUTTONS - 1));
        saveState->sensorMapping.emitEvent(
            synth, MomentaryInputType::KEYBOARD_GATE, 0);
        heldKeys.clear();
      } else if (keyButtons[i].state == WidgetState::HOVER) {
        keyButtons[i].state = WidgetState::INACTIVE;
      }
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; i++) {
      DrawButton(&keyButtons[i], renderer, style, SDL_Color{0, 0, 0, 0});
    }
  }
};
