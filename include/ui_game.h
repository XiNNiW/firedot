#include "SDL_rect.h"
#include "SDL_render.h"
#include "collider.h"
#include "game.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_utils.h"
#include <cstddef>
#include <cstdlib>
struct GameUI {
  Game *game = NULL;
  bool mouseIsDown = false;
  vec2f_t mouseDownPosition;
  vec2f_t mousePosition;
  AxisAlignedBoundingBox shape;

  GameUI(Game *_game) : game(_game) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    game->bounds = AxisAlignedBoundingBox{
        .position = shape.position, .halfSize = shape.halfSize.scale(0.80)};
    game->addWalls();
  };

  void handleFingerMove(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure){};

  void handleFingerDown(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure){};

  void handleFingerUp(const SDL_FingerID &fingerId, const vec2f_t &position,
                      const float pressure){};

  void handleMouseMove(const vec2f_t &mousePosition) {
    this->mousePosition = mousePosition;
  };

  void handleMouseDown(const vec2f_t &mousePosition) {
    mouseIsDown = true;
    this->mouseDownPosition = this->mousePosition = mousePosition;
  };

  void handleMouseUp(const vec2f_t &mousePosition) {
    if (mouseIsDown) {

      auto size = mousePosition.subtract(mouseDownPosition).length();
      auto vx = (rand() / float(RAND_MAX) - 0.5) * 2.0;
      auto vy = (rand() / float(RAND_MAX) - 0.5) * 2.0;
      game->addParticle(
          mouseDownPosition,
          vec2f_t{.x = static_cast<float>(vx), .y = static_cast<float>(vy)},
          size);
    }
    mouseIsDown = false;
    this->mousePosition = mousePosition;
  };

  void draw(SDL_Renderer *renderer, const Style &style) {
    for (auto &gameObject : game->gameObjects) {
      switch (gameObject->type) {

      case GameObject::PARTICLE: {
        auto &particle = gameObject->object.particle;
        auto &destRect = gameObject->getRenderBox();
        SDL_RenderCopy(renderer, style.getParticleTexture(), NULL, &destRect);
        SDL_SetRenderDrawColor(renderer, style.color0.r, style.color0.g,
                               style.color0.b, style.color0.a);
        SDL_RenderDrawRect(renderer, &destRect);
        SDL_SetRenderDrawColor(renderer, style.color1.r, style.color1.g,
                               style.color1.b, style.color1.a);
        // auto colliderRect = gameObject->getCollider()->computeRenderBox();
        // SDL_RenderDrawRect(renderer, &colliderRect);
        // SDL_SetRenderDrawColor(renderer, style.color2.r, style.color2.g,
        //                        style.color2.b, style.color2.a);
        // SDL_RenderDrawPoint(renderer,
        //                    static_cast<int>(gameObject->getPosition().x),
        //                   static_cast<int>(gameObject->getPosition().y));

        break;
      }

      case GameObject::WALL: {
        drawWall(&gameObject->object.wall, renderer, style);
      } break;
      }
    }
  }

  void drawWall(Wall *wall, SDL_Renderer *renderer, const Style &style) {
    switch (wall->collider.type) {
    case Collider::CIRCLE:
      break;
    case Collider::ORIENTED_BOUNDING_BOX: {
      auto &collider = wall->collider.object.orientedBoundingBox;
      auto fakeAABB = AxisAlignedBoundingBox();
      fakeAABB.position = collider.position;
      fakeAABB.halfSize = collider.halfSize;
      DrawBoxOutline(fakeAABB, renderer, style.color2);
      // auto &collider = wall->collider.object.orientedBoundingBox;
      // auto &position = collider.position;
      // auto size = collider.halfSize.scale(2);
      // double rotation = collider.getAngle();
      // auto rect = SDL_Rect{.x = static_cast<int>(position.x),
      //                      .y = static_cast<int>(position.y),
      //                      .w = static_cast<int>(size.x),
      //                      .h = static_cast<int>(size.y)};
      // auto *mTexture =
      //     SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
      //                       SDL_TEXTUREACCESS_TARGET, size.x, size.y);
      // SDL_SetRenderTarget(renderer, mTexture);
      // SDL_SetRenderDrawColor(renderer, style.color2.r, style.color2.g,
      //                        style.color2.b, style.color2.a);
      // SDL_RenderDrawRect(renderer, &rect);
      // SDL_SetRenderTarget(renderer, NULL);
      // auto center = SDL_Point{.x = static_cast<int>(position.x),
      //                         .y = static_cast<int>(position.y)};
      // SDL_RenderCopyEx(renderer, mTexture, &rect, &rect, rotation, &center,
      //                  SDL_RendererFlip::SDL_FLIP_NONE);
      // SDL_DestroyTexture(mTexture);
    }
    case Collider::AXIS_ALIGNED_BOUNDING_BOX:
      DrawBoxOutline(wall->collider.object.axisAlignedBoundingBox, renderer,
                     style.color2);
      break;
    }
  }
};
