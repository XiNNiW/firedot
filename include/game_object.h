#pragma once
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "collider.h"
#include "vector_math.h"
struct Particle {
  vec2f_t velocity = vec2f_t{.x = 0, .y = 0};
  Collider collider;

  Particle(const vec2f_t &_velocity, const Collider &_collider)
      : velocity(_velocity), collider(_collider) {}
  Particle(const Collider &_collider) : collider(_collider) {}
};

struct Wall {
  Collider collider;
  SDL_Color color = {0, 80, 80};
  Wall(const Collider &_collider) : collider(_collider) {}
};

class GameObject {
private:
  vec2f_t position;
  SDL_Rect renderBox;
  const void setColliderPosition(const vec2f_t &position) {
    auto *collider = getCollider();
    collider->setPosition(position);
  }

public:
  enum GameObjectType { PARTICLE, WALL } type;
  union uGameObject {
    Particle particle;
    Wall wall;
    uGameObject(const Particle &p) : particle(p) {}
    uGameObject(const Wall &w) : wall(w) {}
  } object;

  GameObject(const Particle &p)
      : object(p), type(PARTICLE), position(p.collider.getPosition()),
        renderBox(p.collider.computeRenderBox()) {}
  GameObject(const Wall &w)
      : object(w), type(WALL), position(w.collider.getPosition()),
        renderBox(w.collider.computeRenderBox()) {}

  const SDL_Rect &getRenderBox() const { return renderBox; }

  const vec2f_t &getPosition() const { return position; }
  const void setPosition(const vec2f_t &position) {
    this->position = position;
    renderBox.x = position.x - renderBox.w / 2.0;
    renderBox.y = position.y - renderBox.h / 2.0;
    setColliderPosition(position);
  }
  Collider *getCollider() {
    switch (type) {
    case PARTICLE:
      return &object.particle.collider;
      break;
    case WALL:
      return &object.wall.collider;
      break;
    default:
      return NULL;
    }
  }
};
