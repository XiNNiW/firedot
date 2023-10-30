#pragma once

#include <algorithm>
#include <math.h>
struct vec2f_t {

  float x = 0, y = 0;
  inline const float length() const { return float(sqrt((x * x) + (y * y))); }
  inline const float dot(const vec2f_t &other) const {
    return (x * other.x) + (y * other.y);
  }
  inline const vec2f_t subtract(const vec2f_t &other) const {
    return vec2f_t{.x = x - other.x, .y = y - other.y};
  }
  inline const vec2f_t add(const vec2f_t &other) const {
    return vec2f_t{.x = x + other.x, .y = y + other.y};
  }
  inline const vec2f_t norm() const {
    auto l = length();
    return vec2f_t{.x = x / l, .y = y / l};
  }
  inline const vec2f_t scale(float factor) const {
    return vec2f_t{.x = x * factor, .y = y * factor};
  }
  static inline const float clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
  }
  inline const vec2f_t clamp(const vec2f_t &min, const vec2f_t &max) {
    return vec2f_t{.x = clamp(x, min.x, max.x), .y = clamp(y, min.y, max.y)};
  }
};
