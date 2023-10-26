#pragma once
#include "game_object.h"
#include "vector_math.h"
#include <cfloat>
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
  void update(const float deltaTimeSeconds,
              std::vector<GameObject *> *gameObjects) {
    updatePositions(deltaTimeSeconds, gameObjects);
    detectCollisions(gameObjects);
    handleCollisions();
  }

  const std::vector<collision_t> &getCollisions() { return collisions; }
  static inline std::optional<vec2f_t>
  intersection(const CircleCollider &circle1, const CircleCollider &circle2) {
    auto seperationVector = circle1.position.subtract(circle2.position);
    auto distance = seperationVector.length();
    auto sumOfRadii = (circle1.r + circle2.r);
    if (distance > sumOfRadii) {
      return std::nullopt;
    }
    auto overlap = distance - sumOfRadii;
    auto minTranslationVector = seperationVector.norm().scale(overlap);
    return minTranslationVector;
  }
  static inline std::optional<vec2f_t>
  intersection(const CircleCollider &circle, const OrientedBoundingBox &box) {
    auto boxVertices = box.vertices();
    auto axes =
        std::array<vec2f_t, 4>({box.axisX,
                                {.x = -box.axisX.x, .y = -box.axisX.y},
                                box.axisY,
                                {.x = -box.axisY.x, .y = -box.axisY.y}});

    float minOverlap = MAXFLOAT;
    vec2f_t minAxis;

    // for each axis project all vertices and detect overlap
    for (auto &axis : axes) {

      auto toEdge = axis.scale(circle.r);
      auto circleEdgePoint1 = circle.position.add(toEdge);
      auto circleEdgePoint2 = circle.position.subtract(toEdge);
      auto proj1 = circleEdgePoint1.dot(axis);
      auto proj2 = circleEdgePoint2.dot(axis);
      auto circleProjection = projection_t{.minimum = std::min(proj1, proj2),
                                           .maximum = std::max(proj1, proj2)};

      auto boxProjection = projection_t{.minimum = FLT_MAX, .maximum = FLT_MIN};
      for (auto &vertex : boxVertices) {
        auto proj = vertex.dot(axis);
        boxProjection.minimum = std::min(boxProjection.minimum, proj);
        boxProjection.maximum = std::max(boxProjection.maximum, proj);
      }
      auto overlap = circleProjection.overlap(boxProjection);

      if (overlap == 0) {
        return std::nullopt;
      } else if (overlap < minOverlap) {
        minOverlap = overlap;
        minAxis = axis;
      }
    }
    // auto minTranslationVector = minAxis.scale(minOverlap);

    auto minTranslationVector = minAxis.scale(minOverlap);
    return minTranslationVector;
  }

  static inline std::optional<vec2f_t>
  intersection(const OrientedBoundingBox &box1,
               const OrientedBoundingBox &box2) {

    // detect intersection using seperating axis theorem
    auto box1Vertices = box1.vertices();
    auto box2Vertices = box2.vertices();
    constexpr size_t NUM_VERTICES = 4;
    constexpr size_t NUM_AXES = 4;
    auto axes = std::array<vec2f_t, NUM_AXES>(
        {box1.axisX, box1.axisY, box2.axisX, box2.axisY});
    auto minTranslationVector = axes.at(0);
    float minOverlap = MAXFLOAT;

    // for each axis project all vertices and detect overlap
    for (size_t a = 0; a < NUM_AXES; a++) {
      auto axis = axes[a];
      auto minBox1Projection = axis.dot(box1Vertices.at(0));
      // auto maxBox1Projection = minBox1Projection;
      auto box1Projection = projection_t{.minimum = minBox1Projection,
                                         .maximum = minBox1Projection};

      auto minBox2Projection = axis.dot(box2Vertices.at(0));
      // auto maxBox2Projection = minBox2Projection;
      auto box2Projection = projection_t{.minimum = minBox2Projection,
                                         .maximum = minBox2Projection};
      for (size_t i = 1; i < NUM_VERTICES; i++) {
        auto projection1 = axis.dot(box1Vertices[i]);
        auto projection2 = axis.dot(box2Vertices[i]);
        box1Projection.minimum = std::min(box1Projection.minimum, projection1);
        box1Projection.maximum = std::max(box1Projection.maximum, projection1);
        box2Projection.minimum = std::min(box2Projection.minimum, projection2);
        box2Projection.maximum = std::max(box2Projection.maximum, projection2);
      }

      auto overlap = box1Projection.overlap(box2Projection);

      if (overlap == 0) {
        // no overlap! found seperating axis... these shapes are not touching
        return std::nullopt;
      }
      if (overlap < minOverlap) {
        minOverlap = overlap;
        minTranslationVector = axis;
      }
    }

    // no seperating axis was found... return the vector to get the colliding
    // shape out of the other one (minTranslationVector)
    return minTranslationVector.scale(minOverlap);
  }

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
    auto r = d.subtract(n.scale(2 * d.dot(n)));

    // update velocity with some loss
    float loss = 0.9;
    p1->velocity = r.scale(loss);
  }

  // dont need to handle this
  // void interact(Wall *w1, Wall *w2) {}

  void updatePositions(const float deltaTimeSeconds,
                       std::vector<GameObject *> *gameObjects) {
    for (size_t i = 0; i < gameObjects->size(); i++) {
      auto object = gameObjects->at(i);
      switch (object->type) {
      case GameObject::PARTICLE:
        object->object.particle.velocity = object->object.particle.velocity.add(
            gravity.scale(deltaTimeSeconds));
        object->object.particle.update(deltaTimeSeconds, pixelPerMeter);
        break;
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
  void detectCollisions(const std::vector<GameObject *> *gameObjects) {
    collisions.clear();
    for (size_t i = 0; i < gameObjects->size(); i++) {
      for (size_t j = i + 1; j < gameObjects->size(); j++) {

        auto object1 = gameObjects->at(i);
        auto object2 = gameObjects->at(j);

        switch (object1->type) {
        case GameObject::PARTICLE: {
          Particle *particle1 = &object1->object.particle;
          switch (object2->type) {
          case GameObject::PARTICLE: {
            Particle *particle2 = &object2->object.particle;
            auto mvt = particle1->collider.intersection(particle2->collider);
            if (mvt.has_value()) {
              collisions.push_back(
                  collision_t{.object1 = object1,
                              .object2 = object2,
                              .minimumTranslationVector = mvt.value()});
            }
            break;
          }
          case GameObject::WALL: {
            Wall *wall2 = &object2->object.wall;
            auto mvt = particle1->collider.intersection(wall2->collider);
            if (mvt.has_value()) {
              collisions.push_back(
                  collision_t{.object1 = object1,
                              .object2 = object2,
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
                  collision_t{.object1 = object2,
                              .object2 = object1,
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
