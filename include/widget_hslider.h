#pragma once

#include "SDL_log.h"
#include "SDL_render.h"
#include "collider.h"
#include "widget_label.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_utils.h"
#include <string>

struct HSlider {
  WidgetState state = INACTIVE;
  Label label = Label();
  AxisAlignedBoundingBox shape;
};

inline const HSlider MakeHSlider(const std::string &labelText,
                                 AxisAlignedBoundingBox shape,
                                 WidgetState state = INACTIVE) {
  auto labelShape = shape;
  labelShape.halfSize = labelShape.halfSize.scale(0.25);
  labelShape.position.x -= shape.halfSize.x;
  labelShape.position.x += labelShape.halfSize.x;
  labelShape.position.y -= shape.halfSize.y;
  labelShape.position.y += labelShape.halfSize.y;
  return HSlider{
      .state = state, .label = Label(labelShape, labelText), .shape = shape};
}

inline void SetHSliderValue(HSlider *slider, float *sliderValue,
                            const vec2f_t &mousePosition) {
  float width = slider->shape.halfSize.x * 2;
  float relativePosition =
      mousePosition.x - (slider->shape.position.x - slider->shape.halfSize.x);
  SDL_Log("slider %f/%f", relativePosition, width);
  *sliderValue = fmax(fmin(relativePosition / width, 1.0), 0.0);
}

inline bool DoHSliderClick(HSlider *slider, float *sliderValue,
                           const vec2f_t &mousePosition) {
  if (slider->shape.contains(mousePosition)) {
    SetHSliderValue(slider, sliderValue, mousePosition);
    slider->state = ACTIVE;
    return true;
  }
  return false;
}

inline bool DoHSliderDrag(HSlider *slider, float *sliderValue,
                          const vec2f_t &mousePosition) {
  if (slider->state == ACTIVE) {
    SetHSliderValue(slider, sliderValue, mousePosition);
    return true;
  }
  return false;
}

inline void DrawHSlider(HSlider *slider, const float &sliderValue,
                        SDL_Renderer *renderer, const Style &style) {
  auto sliderBounds = ConvertAxisAlignedBoxToSDL_Rect(slider->shape);
  auto dataBounds = sliderBounds;
  dataBounds.w *= sliderValue;
  auto valueColor = style.color1;
  if (slider->state == ACTIVE) {
    valueColor = style.color0;
  }
  SDL_SetRenderDrawColor(renderer, style.inactiveColor.r, style.inactiveColor.g,
                         style.inactiveColor.b, style.inactiveColor.a);

  SDL_RenderFillRect(renderer, &sliderBounds);
  if (slider->state == HOVER) {
    SDL_SetRenderDrawColor(renderer, style.hoverColor.r, style.hoverColor.g,
                           style.hoverColor.b, style.hoverColor.a);
    SDL_RenderDrawRect(renderer, &sliderBounds);
  }
  SDL_SetRenderDrawColor(renderer, valueColor.r, valueColor.g, valueColor.b,
                         valueColor.a);
  SDL_RenderFillRect(renderer, &dataBounds);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

  slider->label.draw(style.getWidgetLabelColor(slider->state),
                     style.getWidgetColor(slider->state), renderer, style);
}
