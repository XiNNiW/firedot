#pragma once

#include "SDL_log.h"
#include "SDL_render.h"
#include "collider.h"
#include "widget_label.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_utils.h"
#include <string>

struct VSlider {
  WidgetState state = INACTIVE;
  std::string labelText;
  float value;
  AxisAlignedBoundingBox shape;
};

inline void SetVSliderValue(VSlider *slider, const vec2f_t &mousePosition) {
  float height = slider->shape.halfSize.y * 2;
  float relativePosition =
      mousePosition.y - (slider->shape.position.y - slider->shape.halfSize.y);
  SDL_Log("slider %f/%f", relativePosition, height);
  slider->value = fmax(fmin(relativePosition / height, 1.0), 0.0);
}

inline bool DoVSliderClick(VSlider *slider, const vec2f_t &mousePosition) {
  if (slider->shape.contains(mousePosition)) {
    SetVSliderValue(slider, mousePosition);
    slider->state = ACTIVE;
    return true;
  }
  return false;
}

inline bool DoVSliderDrag(VSlider *slider, const vec2f_t &mousePosition) {
  if (slider->state == ACTIVE) {
    SetVSliderValue(slider, mousePosition);
    return true;
  }
  return false;
}

inline void DrawVSlider(const VSlider &slider, SDL_Renderer *renderer,
                        const Style &style) {
  auto sliderBounds = ConvertAxisAlignedBoxToSDL_Rect(slider.shape);
  auto dataBounds = sliderBounds;
  dataBounds.h *= slider.value;
  dataBounds.y -= dataBounds.y - slider.value;
  auto valueColor = style.color1;
  if (slider.state == ACTIVE) {
    valueColor = style.color0;
  }
  SDL_SetRenderDrawColor(renderer, style.inactiveColor.r, style.inactiveColor.g,
                         style.inactiveColor.b, style.inactiveColor.a);

  SDL_RenderFillRect(renderer, &sliderBounds);
  SDL_SetRenderDrawColor(renderer, valueColor.r, valueColor.g, valueColor.b,
                         valueColor.a);
  SDL_RenderFillRect(renderer, &dataBounds);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

  DrawLabel(slider.labelText, style.getWidgetLabelColor(slider.state),
            style.getWidgetColor(slider.state), sliderBounds, renderer, style);
}
