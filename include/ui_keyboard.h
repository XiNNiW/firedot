#pragma once

#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "collider.h"
#include "mapping.h"
#include "metaphor.h"
#include "pitch_collection.h"
#include "save_state.h"
#include "sequencer.h"
#include "synthesis.h"
#include "ui_navigation.h"
#include "widget_state.h"
#include "widget_utils.h"
#include "widget_vslider.h"
#include "window.h"
#include <map>
#include <string>

struct KeyboardUI {

  static constexpr size_t SYNTH_SELECTED_RADIO_GROUP_SIZE = 3;
  static constexpr size_t NUM_KEY_BUTTONS = 35;

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
  AxisAlignedBoundingBox shape;

  bool needsDraw = true;
  SDL_Texture *cachedRender = NULL;

  KeyboardUI(Synthesizer<float> *_synth, SaveState *_saveState)
      : synth(_synth), saveState(_saveState) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
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
      auto buttonPosition =
          vec2f_t{.x = static_cast<float>(
                      shape.position.x - (keysPerRow / 2.0) * keySize +
                      keySize / 2 + (i % int(keysPerRow)) * keySize),
                  .y = static_cast<float>(titleBarHeight + keySize / 2.0 +
                                          floor(i / keysPerRow) * keySize)};
      auto buttonHalfSize = vec2f_t{.x = static_cast<float>(keySize / 2),
                                    .y = static_cast<float>(keySize / 2)};
      keyButtons[i] = Button{
          .label = Label(
              {.position = buttonPosition,
               .halfSize = buttonHalfSize.scale(0.35)},
              GetNoteName(
                  saveState->sensorMapping.key +
                  ForceToScale(i, Scales[static_cast<size_t>(
                                      saveState->sensorMapping.scaleType)]))),
          .shape = AxisAlignedBoundingBox{.position = buttonPosition,
                                          .halfSize = buttonHalfSize}};
    }
    needsDraw = true;
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (DoButtonHover(&keyButtons[i], position)) {
        // auto bentNote = synth->noteMap.notes[heldKeys[fingerId]];
        // auto bendDestination = synth->noteMap.notes[i];
        //  synth->bendNote(bentNote, bendDestination);
        saveState->sensorMapping.emitSteppedEvent(synth, KEYBOARD, KEYBOARD_KEY,
                                                  float(i), NUM_KEY_BUTTONS);
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

    needsDraw = true;
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
        // saveState->sensorMapping.emitEvent(
        //     synth, ContinuousInputType::KEYBOARD_KEY,
        //     float(i) / float(NUM_KEY_BUTTONS - 1));

        saveState->sensorMapping.emitSteppedEvent(
            synth, KEYBOARD, ContinuousInputType::KEYBOARD_KEY, float(i),
            NUM_KEY_BUTTONS);
        saveState->sensorMapping.emitEvent(
            synth, KEYBOARD, MomentaryInputType::KEYBOARD_GATE, 1);
      }
    }
    needsDraw = true;
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {

    if (auto buttonIdx = heldKeys[fingerId]) {
      keyButtons[buttonIdx].state = WidgetState::INACTIVE;
      saveState->sensorMapping.emitSteppedEvent(
          synth, KEYBOARD, ContinuousInputType::KEYBOARD_KEY, float(buttonIdx),
          NUM_KEY_BUTTONS);
      saveState->sensorMapping.emitEvent(synth, KEYBOARD,
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

    needsDraw = true;
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {}

  inline void handleMouseDown(const vec2f_t &mousePosition) {}

  inline void handleMouseUp(const vec2f_t &mousePosition) {

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (keyButtons[i].state == WidgetState::ACTIVE) {
        keyButtons[i].state = WidgetState::INACTIVE;

        saveState->sensorMapping.emitEvent(
            synth, KEYBOARD, MomentaryInputType::KEYBOARD_GATE, 0);
        heldKeys.clear();
      } else if (keyButtons[i].state == WidgetState::HOVER) {
        keyButtons[i].state = WidgetState::INACTIVE;
      }
    }
    needsDraw = true;
  }
  inline void _draw(SDL_Renderer *renderer, const Style &style) {

    for (size_t i = 0; i < NUM_KEY_BUTTONS; i++) {
      DrawButton(&keyButtons[i], renderer, style, SDL_Color{0, 0, 0, 0});
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    // if (needsDraw) {
    //   if (cachedRender == NULL) {
    //     cachedRender = SDL_CreateTexture(
    //         renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
    //         ActiveWindow::size.x, ActiveWindow::size.y);
    //   }
    //   auto err = SDL_SetRenderTarget(renderer, cachedRender);
    //   if (err < 0) {
    //     SDL_Log("%s", SDL_GetError());
    //   }
    _draw(renderer, style);
    //     SDL_SetRenderTarget(renderer, NULL);
    //     needsDraw = false;
    //   }
    //   SDL_Rect renderRect = ConvertAxisAlignedBoxToSDL_Rect(shape);
    //   SDL_RenderCopy(renderer, cachedRender, NULL, NULL);
  }

  ~KeyboardUI() { SDL_DestroyTexture(cachedRender); }
};
