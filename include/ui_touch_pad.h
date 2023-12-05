#pragma once

#include "SDL_render.h"
#include "SDL_touch.h"
#include "collider.h"
#include "sensor.h"
#include "synthesis.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_seperator.h"
#include "widget_style.h"
struct TouchPadUI {
  static constexpr int NUM_FINGERS = 10;
  vec2f_t fingerPositions[NUM_FINGERS] = {};
  bool fingerActivity[NUM_FINGERS] = {false, false, false, false, false,
                                      false, false, false, false, false};
  Synthesizer<float> *synth;
  SensorMapping<float> *mapping;
  AxisAlignedBoundingBox shape;
  TouchPadUI(Synthesizer<float> *_synth, SensorMapping<float> *_mapping)
      : synth(_synth), mapping(_mapping) {}
  inline void buildLayout(const AxisAlignedBoundingBox &shape){};

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    fingerPositions[fingerId] = position;
    // send messages to update synth parameters
  };

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    fingerPositions[fingerId] = position;
    fingerActivity[fingerId] = true;
    // send triggers
  };

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    fingerPositions[fingerId] = position;
    fingerActivity[fingerId] = false;
  };

  inline void handleMouseMove(const vec2f_t &mousePosition){};

  inline void handleMouseDown(const vec2f_t &mousePosition){};

  inline void handleMouseUp(const vec2f_t &mousePosition){};

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    for (int i = 0; i < NUM_FINGERS; ++i) {
      if (fingerActivity[i]) {
        auto position = fingerPositions[i];
        auto size = 100;
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
  };
};
