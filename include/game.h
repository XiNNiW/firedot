#pragma once

#include "collider.h"
#include "game_object.h"
#include "physics.h"
#include "sensor.h"
#include "synthesis.h"
#include "vector_math.h"
#include <algorithm>
#include <memory>

struct Game {
  const float MIN_COLLISION_VELOCITY = 0.1;
  const float MAX_PARTICLE_SIZE = 100;
  Physics physics;
  std::vector<std::unique_ptr<GameObject>> gameObjects;
  InputMapping<float> *mapping = NULL;
  Synthesizer<float> *synth = NULL;
  std::vector<float> activeGateTimes;
  float gateWidthSeconds = 0.1;
  AxisAlignedBoundingBox bounds;

  Game(InputMapping<float> *_mapping, Synthesizer<float> *_synth)
      : mapping(_mapping), synth(_synth) {}

  inline void update(float secondsSinceLastUpdate) {
    physics.update(secondsSinceLastUpdate, &gameObjects);
    auto biggerBounds = bounds;
    biggerBounds.halfSize = biggerBounds.halfSize.scale(2);

    gameObjects.erase(remove_if(gameObjects.begin(), gameObjects.end(),
                                [this, &biggerBounds](auto &gameObject) {
                                  auto position = gameObject->getPosition();
                                  return !biggerBounds.contains(position);
                                }),
                      gameObjects.end());

    for (size_t i = 0; i < activeGateTimes.size(); i++) {
      activeGateTimes[i] += secondsSinceLastUpdate;
      if (activeGateTimes[i] >= gateWidthSeconds) {
        mapping->emitEvent(synth, MomentaryInputType::COLLISION, false);
      }
    }
    activeGateTimes.erase(remove_if(activeGateTimes.begin(),
                                    activeGateTimes.end(),
                                    [this](auto gateTime) {
                                      return (gateTime >= gateWidthSeconds);
                                    }),
                          activeGateTimes.end());

    for (auto &collision : physics.getCollisions()) {
      auto obj1 = collision.object1;
      auto obj2 = collision.object2;

      switch (obj1->type) {
      case GameObject::PARTICLE: {
        switch (obj2->type) {
        case GameObject::PARTICLE: {
          auto p1 = obj1->object.particle;
          auto p2 = obj2->object.particle;
          auto collisionVelocity = p2.velocity.subtract(p1.velocity).length() >
                                   MIN_COLLISION_VELOCITY;
          if (collisionVelocity) {
            auto v1 = lerp<float>(1, 127, p2.velocity.length() / 4.0);
            auto v2 = lerp<float>(1, 127, p2.velocity.length() / 4.0);

            mapping->emitEvent(synth, ContinuousInputType::COLLISION_VELOCITY,
                               collisionVelocity);
            mapping->emitEvent(synth, ContinuousInputType::COLLISION_POSITION_X,
                               p1.collider.getPosition().x);

            mapping->emitEvent(synth, ContinuousInputType::COLLISION_POSITION_Y,
                               p1.collider.getPosition().y);
            mapping->emitEvent(synth, MomentaryInputType::COLLISION, true);
            activeGateTimes.push_back(0);
          }
          break;
        }
        case GameObject::WALL: {
          auto p1 = obj1->object.particle;
          auto collisionVelocity =
              p1.velocity.length() > MIN_COLLISION_VELOCITY;
          if (collisionVelocity) {

            mapping->emitEvent(synth, ContinuousInputType::COLLISION_VELOCITY,
                               collisionVelocity);
            mapping->emitEvent(synth, ContinuousInputType::COLLISION_POSITION_X,
                               p1.collider.getPosition().x);

            mapping->emitEvent(synth, ContinuousInputType::COLLISION_POSITION_Y,
                               p1.collider.getPosition().y);
            mapping->emitEvent(synth, MomentaryInputType::COLLISION, true);
            activeGateTimes.push_back(0);
          }

          break;
        }
        }
        break;
      }
      case GameObject::WALL: {

        switch (obj2->type) {
        case GameObject::PARTICLE: {
          break;
        }
        case GameObject::WALL: {
          break;
        }
        }
        break;
      }
      }
    }
  }

  inline void addWalls() {
    destroyAllWalls();
    auto bottomWall = new GameObject(Wall(AxisAlignedBoundingBox{
        vec2f_t{.x = bounds.position.x,
                .y = bounds.position.y + bounds.halfSize.y},
        vec2f_t{.x = bounds.halfSize.x, .y = bounds.halfSize.y / 32}}));

    auto leftWall = new GameObject(Wall(AxisAlignedBoundingBox{
        {.x = bounds.position.x - bounds.halfSize.x, .y = bounds.position.y},
        {.x = bounds.halfSize.y / 32, .y = bounds.halfSize.y}}));

    auto rightWall = new GameObject(Wall(AxisAlignedBoundingBox{
        {.x = bounds.position.x + bounds.halfSize.x, .y = bounds.position.y},
        {.x = bounds.halfSize.y / 32, .y = bounds.halfSize.y}}));

    gameObjects.push_back(std::unique_ptr<GameObject>(std::move(bottomWall)));
    gameObjects.push_back(std::unique_ptr<GameObject>(std::move(leftWall)));
    gameObjects.push_back(std::unique_ptr<GameObject>(std::move(rightWall)));
  }

  inline void addParticle(const vec2f_t &pos, const vec2f_t &velocity,
                          float size) {
    auto gameObject = new GameObject(
        Particle(velocity, CircleCollider{.position = pos, .radius = size}));
    gameObjects.push_back(std::unique_ptr<GameObject>(std::move(gameObject)));
  }

  inline void destroyAllPartcles() {
    gameObjects.erase(std::remove_if(gameObjects.begin(), gameObjects.end(),
                                     [](auto &gameObject) {
                                       return gameObject->type ==
                                              GameObject::PARTICLE;
                                     }),
                      gameObjects.end());
  }

  inline void destroyAllWalls() {
    gameObjects.erase(std::remove_if(gameObjects.begin(), gameObjects.end(),
                                     [](auto &gameObject) {
                                       return gameObject->type ==
                                              GameObject::WALL;
                                     }),
                      gameObjects.end());
  }
};
