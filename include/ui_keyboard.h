#pragma once

#include "collider.h"
#include "metaphor.h"
#include "sequencer.h"
#include "synthesis.h"
#include "ui_abstract.h"
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
  static constexpr size_t NUM_KEY_BUTTONS = 24 + 11;

  float notes[NUM_KEY_BUTTONS] = {
      36,         37,         38,         39,         40,         36 + 5,
      37 + 5,     38 + 5,     39 + 5,     40 + 5,     36 + 2 * 5, 37 + 2 * 5,
      38 + 2 * 5, 39 + 2 * 5, 40 + 2 * 5, 36 + 3 * 5, 37 + 3 * 5, 38 + 3 * 5,
      39 + 3 * 5, 40 + 3 * 5, 36 + 4 * 5, 37 + 4 * 5, 38 + 4 * 5, 39 + 4 * 5,
      40 + 4 * 5, 36 + 5 * 5, 37 + 5 * 5, 38 + 5 * 5, 39 + 5 * 5, 40 + 5 * 5,
      36 + 6 * 5, 37 + 6 * 5, 38 + 6 * 5, 39 + 6 * 5, 40 + 6 * 5,
  };
  Synthesizer<float> *synth = NULL;

  std::map<SDL_FingerID, int> heldKeys;
  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  Button keyButtons[NUM_KEY_BUTTONS];

  float synthSelectWidth = 150;
  float synthSelectHeight = 50;
  float titleBarHeight = 100;
  float buttonMargin = 5;
  float topMargin = 15;

  KeyboardUI(Synthesizer<float> *_synth) : synth(_synth) {}

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
    // titleBarHeight = height / 24.0;
    // navigationUI->pages.shape = {.position{.x = 0, .y = 0},
    //.halfSize = {.x = width, .y = titleBarHeight }
    //};

    //  soundEditMenuButton = Button{
    //      .labelText = "sound edit",
    //      .shape = AxisAlignedBoundingBox{
    //          .position = vec2f_t{.x = static_cast<float>(pageMargin +
    //                                                      synthSelectWidth /
    //                                                      2),
    //                              .y = static_cast<float>(pageMargin +
    //                                                      synthSelectHeight /
    //                                                      2)},
    //          .halfSize =
    //              vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
    //                      .y = static_cast<float>(synthSelectHeight / 2)}}};
    //  mappingMenuButton = Button{
    //      .labelText = "sensor mapping",
    //      .shape = AxisAlignedBoundingBox{
    //          .position = vec2f_t{.x = width - synthSelectWidth / 2 -
    //          pageMargin,
    //                              .y = static_cast<float>(pageMargin +
    //                                                      synthSelectHeight /
    //                                                      2)},
    //          .halfSize =
    //              vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
    //                      .y = static_cast<float>(synthSelectHeight / 2)}}};

    // auto topBarHeight = synthSelectHeight + (1.5 * buttonMargin) +
    // pageMargin;
    //  auto keySize = width / 4.5;
    auto keySize = width / 5.5;
    auto keyboardStartPositionX = 100;
    float keysPerRow = 5;

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {

      keyButtons[i] = Button{
          .labelText = GetNoteName(notes[i]),
          .shape = AxisAlignedBoundingBox{
              .position = vec2f_t{.x = static_cast<float>(
                                      pageMargin + keySize / 2.0 +
                                      (i % int(keysPerRow)) * (keySize)),
                                  .y = static_cast<float>(
                                      titleBarHeight + keySize / 2.0 +
                                      floor(i / keysPerRow) * (keySize))},
              .halfSize = vec2f_t{.x = static_cast<float>(keySize / 2),
                                  .y = static_cast<float>(keySize / 2)}}};
    }
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (DoButtonHover(&keyButtons[i], position)) {
        auto bentNote = notes[heldKeys[fingerId]];
        auto bendDestination = notes[i];
        synth->bendNote(bentNote, bendDestination);
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
    //  DoButtonHover(&soundEditMenuButton, position);
    //  DoButtonHover(&mappingMenuButton, position);
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      // evaluate clicks
      if (DoButtonClick(&keyButtons[i], position, WidgetState::ACTIVE)) {
        // play sound
        auto note = notes[i];
        heldKeys[fingerId] = i;
        synth->note(note, pressure * 127);
      }
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {

    if (auto buttonIdx = heldKeys[fingerId]) {
      keyButtons[buttonIdx].state = WidgetState::INACTIVE;
      synth->note(notes[buttonIdx], 0);
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

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    // navigationUI->handleMouseDown(mousePosition);
    //  if (DoButtonClick(&soundEditMenuButton, mousePosition)) {
    //    navigation->page = Navigation::SOUND_EDIT;
    //  };

    //  if (DoButtonClick(&mappingMenuButton, mousePosition)) {
    //    navigation->page = Navigation::MAPPING;
    //  };

    //  for (size_t i = 0; i < keyboardWidget->keyButtons.size(); ++i) {
    //    // evaluate clicks
    //    if (UpdateButton(&keyboardWidget->keyButtons[i], mousePosition,
    //                     UIState::ACTIVE)) {
    //      // play sound
    //      synth->note(notes[i], 127);
    //    }
    //  }
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {

    // mappingMenuButton.state = INACTIVE;

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (keyButtons[i].state == WidgetState::ACTIVE) {
        keyButtons[i].state = WidgetState::INACTIVE;
        synth->note(notes[i], 0);
        heldKeys.clear();
      } else if (keyButtons[i].state == WidgetState::HOVER) {
        keyButtons[i].state = WidgetState::INACTIVE;
      }
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    //   DrawButton(soundEditMenuButton, renderer, style);
    //   DrawButton(mappingMenuButton, renderer, style);

    for (size_t i = 0; i < NUM_KEY_BUTTONS; i++) {
      DrawButton(keyButtons[i], renderer, style);
    }
  }
};
