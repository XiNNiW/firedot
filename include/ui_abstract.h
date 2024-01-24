#pragma once
#include "vector_math.h"

class AbstractUI {
  virtual void handleMouseDown(const vec2f_t &mousePosition) = 0;
  virtual void handleMouseUp(const vec2f_t &mousePosition) = 0;
  virtual void handleMouseMove(const vec2f_t &mousePosition) = 0;
};
