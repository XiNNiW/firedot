#pragma once

#include "SDL_render.h"
#include "SDL_touch.h"
#include "collider.h"
#include "mapping.h"
#include "metaphor.h"
#include "save_state.h"
#include "synthesis.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_seperator.h"
#include "widget_style.h"
#include "widget_utils.h"

struct TouchPadUI {
  vec2f_t fingerPosition = {0, 0};
  bool fingerActivity = false;
  Synthesizer<float> *synth;
  SaveState *saveState;
  AxisAlignedBoundingBox shape;
  TouchPadUI(Synthesizer<float> *_synth, SaveState *_saveState)
      : synth(_synth), saveState(_saveState) {}
  inline void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
  };

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure){
      // send messages to update synth parameters
  };

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure){};

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure){

  };

  inline const float computeNormalizedYTouchPosition(float y) {
    return 1 - (y - (shape.position.y - shape.halfSize.y)) /
                   (shape.halfSize.y * 2.0);
  }

  inline void handleMouseMove(vec2f_t mousePosition) {
    mousePosition.x =
        fmin(fmax(mousePosition.x, shape.position.x - shape.halfSize.x),
             shape.halfSize.x + shape.position.x);
    mousePosition.y =
        fmin(fmax(mousePosition.y, shape.position.y - shape.halfSize.y),
             shape.halfSize.y + shape.position.y);
    fingerPosition = mousePosition;
    saveState->sensorMapping.emitEvent(
        synth, TOUCH_PAD, ContinuousInputType::TOUCH_X_POSITION,
        mousePosition.x / (shape.halfSize.x * 2.0));
    saveState->sensorMapping.emitEvent(
        synth, TOUCH_PAD, ContinuousInputType::TOUCH_Y_POSITION,
        computeNormalizedYTouchPosition(mousePosition.y));
  };

  inline void handleMouseDown(vec2f_t mousePosition) {
    if (!shape.contains(mousePosition))
      return;
    mousePosition.x =
        fmin(fmax(mousePosition.x, shape.position.x - shape.halfSize.x),
             shape.halfSize.x + shape.position.x);
    mousePosition.y =
        fmin(fmax(mousePosition.y, shape.position.y - shape.halfSize.y),
             shape.halfSize.y + shape.position.y);
    fingerPosition = mousePosition;
    fingerActivity = true;
    // send triggers
    saveState->sensorMapping.emitEvent(
        synth, TOUCH_PAD, ContinuousInputType::TOUCH_X_POSITION,
        fingerPosition.x / (shape.halfSize.x * 2.0));
    saveState->sensorMapping.emitEvent(
        synth, TOUCH_PAD, ContinuousInputType::TOUCH_Y_POSITION,
        computeNormalizedYTouchPosition(fingerPosition.y));
    saveState->sensorMapping.emitEvent(synth, TOUCH_PAD,
                                       MomentaryInputType::TOUCH_PAD_GATE, 1);
  };

  inline void handleMouseUp(const vec2f_t &mousePosition) {
    fingerActivity = false;
    saveState->sensorMapping.emitEvent(
        synth, TOUCH_PAD, ContinuousInputType::TOUCH_X_POSITION,
        fingerPosition.x / (shape.halfSize.x * 2.0));
    saveState->sensorMapping.emitEvent(
        synth, TOUCH_PAD, ContinuousInputType::TOUCH_Y_POSITION,
        computeNormalizedYTouchPosition(fingerPosition.y));
    saveState->sensorMapping.emitEvent(synth, TOUCH_PAD,
                                       MomentaryInputType::TOUCH_PAD_GATE, 0);
  };

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    if (fingerActivity) {
      auto position = fingerPosition;
      auto size = 200;
      auto fingerRect = SDL_Rect{.x = static_cast<int>(position.x - size / 2.0),
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

    //   DrawBoxOutline(
    //       ConvertAxisAlignedBoxToSDL_Rect(AxisAlignedBoundingBox{
    //           .position = shape.position, .halfSize =
    //           shape.halfSize.scale(0.9)}),
    //       renderer, style.color0);
  };
};
