#pragma once
#include "SDL_pixels.h"
#include "collider.h"
struct Particle {
  vec2f_t velocity = vec2f_t{.x = 0, .y = 0};
  Collider collider;

  Particle(const Collider &_collider) : collider(_collider) {}
  inline void update(const float &deltaTimeSeconds,
                     const float &pixelsPerMeter) {
    collider.setPosition(collider.getPosition().add(
        velocity.scale(deltaTimeSeconds).scale(pixelsPerMeter)));
  }
};

struct Wall {
  Collider collider;
  std::array<float, 4> notes = {36 + 24, 40 + 24, 43 + 24, 47 + 24};
  SDL_Color color = {0, 80, 80};
  Wall(const Collider &_collider) : collider(_collider) {}
};

struct Button {
  CircleCollider collider;
  bool isPressed;
  Button() {}
};

struct GameObject {

  enum GameObjectType { PARTICLE, WALL } type;
  union uGameObject {
    Particle particle;
    Wall wall;
    uGameObject(const Particle &p) : particle(p) {}
    uGameObject(const Wall &w) : wall(w) {}
  } object;

  GameObject(const Particle &p) : object(p), type(PARTICLE) {}
  GameObject(const Wall &w) : object(w), type(WALL) {}
};
