#pragma once

#include "widget_button.h"
#include "widget_icon.h"
struct IconButton {
  Icon icon;
};

inline void DrawIconButton(IconButton *button, SDL_Renderer *renderer,
                           const Style &style) {
  DrawButton(button) {}
}
