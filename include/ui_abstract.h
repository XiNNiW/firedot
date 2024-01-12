#pragma once

#include "SDL_render.h"
#include "SDL_touch.h"
#include "collider.h"
#include "widget_style.h"
class AbstractUI {
public:
  AxisAlignedBoundingBox shape;
  virtual inline void refreshLayout() { buildLayout(shape); }
  virtual void buildLayout(const AxisAlignedBoundingBox &shape) = 0;

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position,
                                const float pressure) = 0;

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position,
                                const float pressure) = 0;

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position,
                              const float pressure) = 0;

  virtual void handleMouseMove(const vec2f_t &mousePosition) = 0;

  virtual void handleMouseDown(const vec2f_t &mousePosition) = 0;

  virtual void handleMouseUp(const vec2f_t &mousePosition) = 0;

  virtual void draw(SDL_Renderer *renderer, const Style &style) = 0;
};
