#pragma once
#include "collider.h"
#include "game_object.h"
#include "vector_math.h"
#include <cfloat>
#include <memory>
#include <optional>
#include <vector>

struct collision_t {
  GameObject *object1;
  GameObject *object2;
  vec2f_t minimumTranslationVector;
};

class Physics {
public:
  vec2f_t gravity = vec2f_t{.x = 0, .y = 0};
  double pixelPerMeter = 1000;
  void update(const double deltaTimeSeconds,
              std::vector<std::unique_ptr<GameObject>> *gameObjects) {
    updatePositions(deltaTimeSeconds, gameObjects);
    detectCollisions(gameObjects);
    handleCollisions();
  }

  const std::vector<collision_t> &getCollisions() { return collisions; }

private:
  struct projection_t {
    float minimum = 0, maximum = 0;
    inline const float overlap(const projection_t &other) {

      // return std::max(float(0), std::min(maximum, other.maximum) -
      //                               std::max(minimum, other.minimum));
      if ((maximum > other.minimum) && (minimum < other.maximum)) {
        return maximum - other.minimum;
      } else if ((minimum < other.maximum) && (maximum > other.minimum)) {
        return other.maximum - minimum;
      } else {
        return 0;
      }
    }
  };
  std::vector<collision_t> collisions;
  void interact(Particle *p1, Particle *p2,
                const vec2f_t &minTranslationVector) {
    // handle particle particle collision
    // move them outside one another
    auto xi1 = p1->collider.getPosition();
    auto xi2 = p2->collider.getPosition();

    p1->collider.setPosition(
        p1->collider.getPosition().subtract(minTranslationVector.scale(0.5)));
    p2->collider.setPosition(
        p2->collider.getPosition().add(minTranslationVector.scale(0.5)));

    // figure out transfer of momentuum
    const auto density = 1.0;
    double m1 = density * (double(p1->collider.getArea()));
    double m2 = density * (double(p2->collider.getArea()));
    double massSum = m1 + m2;
    auto v1 = p1->velocity;
    auto v2 = p2->velocity;
    auto x1 = p1->collider.getPosition();
    auto x2 = p2->collider.getPosition();
    auto x1_minus_x2 = x1.subtract(x2).scale(1.0 / pixelPerMeter);
    p1->velocity = v1.subtract(x1_minus_x2.scale(
        (2.0 * m2 / massSum) *
        (v1.subtract(v2).dot(x1_minus_x2) / x1_minus_x2.length())));
    auto x2_minus_x1 = x2.subtract(x1).scale(1.0 / pixelPerMeter);
    p2->velocity = v2.subtract(x2_minus_x1.scale(
        (2.0 * m1 / massSum) * v2.subtract(v1).dot(x2_minus_x1) /
        x2_minus_x1.length()));
  }
  void interact(Particle *p1, Wall *w2, const vec2f_t &minTranslationVector) {

    //  handle particle wall collision
    //  move it outside the wall
    p1->collider.setPosition(
        p1->collider.getPosition().subtract(minTranslationVector));

    // compute reflected velocity (r) from incidence velocity (d)
    // n = normal of surface
    // r=d−2(d⋅n)n
    auto n = minTranslationVector.norm();
    auto d = p1->velocity;
    auto r = d.subtract(n.scale(2.0 * d.dot(n)));

    // update velocity with some loss
    float loss = 0.9;
    p1->velocity = r.scale(loss);
  }

  // dont need to handle this
  // void interact(Wall *w1, Wall *w2) {}

  void updatePositions(const double deltaTimeSeconds,
                       std::vector<std::unique_ptr<GameObject>> *gameObjects) {
    for (size_t i = 0; i < gameObjects->size(); i++) {
      auto &object = gameObjects->at(i);
      switch (object->type) {
      case GameObject::PARTICLE: {
        object->object.particle.velocity = object->object.particle.velocity.add(
            gravity.scale(deltaTimeSeconds));
        auto *collider = object->getCollider();
        object->setPosition(collider->getPosition().add(
            object->object.particle.velocity.scale(deltaTimeSeconds)
                .scale(pixelPerMeter)));
        break;
      }
      case GameObject::WALL:
        break;
      }
    }
  }
  void handleCollisions() {
    for (auto &collision : collisions) {
      auto object1 = collision.object1;
      auto object2 = collision.object2;

      switch (object1->type) {
      case GameObject::PARTICLE: {
        auto particle1 = &object1->object.particle;
        switch (object2->type) {
        case GameObject::PARTICLE: {
          interact(particle1, &object2->object.particle,
                   collision.minimumTranslationVector);
          break;
        }
        case GameObject::WALL: {
          interact(particle1, &object2->object.wall,
                   collision.minimumTranslationVector);
          break;
        }
        }
        break;
      }
      case GameObject::WALL: {
        auto wall1 = &object1->object.wall;
        switch (object2->type) {
        case GameObject::PARTICLE: {
          interact(&object2->object.particle, wall1,
                   collision.minimumTranslationVector);
        } break;
        case GameObject::WALL:
          break;
        }
        break;
      }
      }
    }
  }
  void detectCollisions(
      const std::vector<std::unique_ptr<GameObject>> *gameObjects) {
    collisions.clear();
    for (size_t i = 0; i < gameObjects->size(); i++) {
      for (size_t j = i + 1; j < gameObjects->size(); j++) {

        auto &object1 = gameObjects->at(i);
        auto &object2 = gameObjects->at(j);

        switch (object1->type) {
        case GameObject::PARTICLE: {
          Particle *particle1 = &object1->object.particle;
          switch (object2->type) {
          case GameObject::PARTICLE: {
            Particle *particle2 = &object2->object.particle;
            auto mvt = particle1->collider.intersection(particle2->collider);
            if (mvt.has_value()) {
              collisions.push_back(
                  collision_t{.object1 = object1.get(),
                              .object2 = object2.get(),
                              .minimumTranslationVector = mvt.value()});
            }
            break;
          }
          case GameObject::WALL: {
            Wall *wall2 = &object2->object.wall;
            auto mvt = particle1->collider.intersection(wall2->collider);
            if (mvt.has_value()) {
              collisions.push_back(
                  collision_t{.object1 = object1.get(),
                              .object2 = object2.get(),
                              .minimumTranslationVector = mvt.value()});
            }
            break;
          }
          }
          break;
        }
        case GameObject::WALL: {
          auto wall1 = &object1->object.wall;
          switch (object2->type) {
          case GameObject::PARTICLE: {
            Particle *particle2 = &object2->object.particle;
            auto mvt = particle2->collider.intersection(wall1->collider);
            if (mvt.has_value()) {
              collisions.push_back(
                  collision_t{.object1 = object2.get(),
                              .object2 = object1.get(),
                              .minimumTranslationVector = mvt.value()});
            }

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
  }
};
