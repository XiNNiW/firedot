#pragma once

#include "SDL_render.h"
#include "SDL_touch.h"
#include "collider.h"
#include "save_state.h"
#include "sensor.h"
#include "synthesis.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_seperator.h"
#include "widget_style.h"
#include "widget_utils.h"

struct TouchPadUI {
  static constexpr int NUM_FINGERS = 10;
  vec2f_t fingerPositions[NUM_FINGERS] = {};
  bool fingerActivity[NUM_FINGERS] = {false, false, false, false, false,
                                      false, false, false, false, false};
  Synthesizer<float> *synth;
  SaveState *saveState;
  AxisAlignedBoundingBox shape;
  TouchPadUI(Synthesizer<float> *_synth, SaveState *_saveState)
      : synth(_synth), saveState(_saveState) {}
  inline void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
  };

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    // fingerPositions[fingerId] = position;
    saveState->sensorMapping.emitEvent(synth,
                                       ContinuousInputType::TOUCH_X_POSITION,
                                       position.x / (shape.halfSize.x * 2.0));
    saveState->sensorMapping.emitEvent(synth,
                                       ContinuousInputType::TOUCH_Y_POSITION,
                                       position.y / (shape.halfSize.y * 2.0));
    // send messages to update synth parameters
  };

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    fingerPositions[fingerId] = position;
    fingerActivity[fingerId] = true;
    // send triggers
    saveState->sensorMapping.emitEvent(synth,
                                       ContinuousInputType::TOUCH_X_POSITION,
                                       position.x / (shape.halfSize.x * 2.0));
    saveState->sensorMapping.emitEvent(synth,
                                       ContinuousInputType::TOUCH_Y_POSITION,
                                       position.y / (shape.halfSize.y * 2.0));
    saveState->sensorMapping.emitEvent(synth,
                                       MomentaryInputType::TOUCH_PAD_GATE, 1);

    // saveState->sensorMapping.noteOn(synth, InputType::TOUCH_Y_POSITION, float
    // value)
  };

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    fingerActivity[fingerId] = false;
    saveState->sensorMapping.emitEvent(
        synth, ContinuousInputType::TOUCH_X_POSITION,
        fingerPositions[fingerId].x / (shape.halfSize.x * 2.0));
    saveState->sensorMapping.emitEvent(
        synth, ContinuousInputType::TOUCH_Y_POSITION,
        fingerPositions[fingerId].y / (shape.halfSize.y * 2.0));
    saveState->sensorMapping.emitEvent(synth,
                                       MomentaryInputType::TOUCH_PAD_GATE, 0);
    // saveState->sensorMapping.noteOff(synth, InputType::KEYBOARD_KEY, float
    // value)
  };

  inline void handleMouseMove(const vec2f_t &mousePosition){};

  inline void handleMouseDown(const vec2f_t &mousePosition){};

  inline void handleMouseUp(const vec2f_t &mousePosition){};

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    for (int i = 0; i < NUM_FINGERS; ++i) {
      if (fingerActivity[i]) {
        auto position = fingerPositions[i];
        auto size = 200;
        auto fingerRect =
            SDL_Rect{.x = static_cast<int>(position.x - size / 2.0),
                     .y = static_cast<int>(position.y - size / 2.0),
                     .w = size,
                     .h = size};
        DrawBoxOutline(fingerRect, renderer, style.color0);
        DrawLine({.x = shape.position.x - shape.halfSize.x, .y = position.y},
                 {.x = shape.position.x + shape.halfSize.x, .y = position.y},
                 renderer, style.color0);
        DrawLine({.x = position.x, .y = shape.position.y - shape.halfSize.y},
                 {.x = position.x, .y = shape.position.y + shape.halfSize.y},
                 renderer, style.color0);
      }
    }

    //   DrawBoxOutline(
    //       ConvertAxisAlignedBoxToSDL_Rect(AxisAlignedBoundingBox{
    //           .position = shape.position, .halfSize =
    //           shape.halfSize.scale(0.9)}),
    //       renderer, style.color0);
  };
};
