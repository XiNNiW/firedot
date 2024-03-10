#pragma once
#include "SDL_error.h"
#include "SDL_render.h"
#include "widget_style.h"
#include "window.h"
template <typename Derived> struct CachingRenderUIDecorator : public Derived {

  bool needsDraw = true;
  SDL_Texture *cachedRender = NULL;
  ~CachingRenderUIDecorator() { SDL_DestroyTexture(cachedRender); }

  inline Derived *getDecoratedClass() { return static_cast<Derived>(this); }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    if (needsDraw) {
      if (cachedRender == NULL) {
        cachedRender = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
            ActiveWindow::size.x, ActiveWindow::size.y);
      }
      auto err = SDL_SetRenderTarget(renderer, cachedRender);
      if (err < 0) {
        SDL_Log("%s", SDL_GetError());
      }

      static_cast<Derived>(this)->draw(renderer, style);

      SDL_SetRenderTarget(renderer, NULL);
      needsDraw = false;
    }
    SDL_Rect renderRect =
        ConvertAxisAlignedBoxToSDL_Rect(static_cast<Derived>(this)->shape);
    SDL_RenderCopy(renderer, cachedRender, NULL, NULL);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    static_cast<Derived>(this)->handleFingerMove(fingerId, position, pressure);
    needsDraw = true;
  }
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    static_cast<Derived>(this)->handleFingerDown(fingerId, position, pressure);
    needsDraw = true;
  }
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    static_cast<Derived>(this)->handleFingerUp(fingerId, position, pressure);
    needsDraw = true;
  }
  inline void handleMouseMove(const vec2f_t &mousePosition) {
    static_cast<Derived>(this)->handleMouseMove(mousePosition);
    needsDraw = true;
  }
  inline void handleMouseDown(const vec2f_t &mousePosition) {
    static_cast<Derived>(this)->handleMouseDown(mousePosition);
    needsDraw = true;
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {
    static_cast<Derived>(this)->handleMouseUp(mousePosition);
    needsDraw = true;
  }
};
