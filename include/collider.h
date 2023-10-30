#pragma once
#include "vector_math.h"
#include <array>
#include <cfloat>
#include <optional>

struct CircleCollider {
  vec2f_t position;
  float radius;
  inline const bool contains(const vec2f_t &point) {
    return position.subtract(point).length() <= radius;
  }
};

struct OrientedBoundingBox {
  vec2f_t position, axisX, axisY, halfSize;
  OrientedBoundingBox() {}
  OrientedBoundingBox(const vec2f_t &_position, const vec2f_t &_axisX,
                      const vec2f_t &_axisY, const vec2f_t &_halfSize)
      : position(_position), axisX(_axisX), axisY(_axisY), halfSize(_halfSize) {
  }
  OrientedBoundingBox(vec2f_t pos, vec2f_t size, float rotation)
      : position(pos) {
    halfSize = vec2f_t{.x = size.x / float(2.0), .y = size.y / float(2.0)};
    axisX = vec2f_t{.x = cos(rotation), .y = sin(rotation)}.norm();
    axisY = vec2f_t{.x = -sin(rotation), .y = cos(rotation)}.norm();
  }
  inline const std::array<vec2f_t, 4> vertices() const {
    auto xProj = position.dot(axisX);
    auto yProj = position.dot(axisY);
    return {
        vec2f_t{.x = xProj - halfSize.x, .y = yProj - halfSize.y},
        vec2f_t{.x = xProj - halfSize.x, .y = yProj + halfSize.y},
        vec2f_t{.x = xProj + halfSize.x, .y = yProj - halfSize.y},
        vec2f_t{.x = xProj + halfSize.x, .y = yProj + halfSize.y},
    };
  }
  inline const bool contains(const vec2f_t &point) {
    auto dist = point.subtract(position);
    auto xProj = dist.dot(axisX);
    auto yProj = dist.dot(axisY);
    return (abs(xProj) <= halfSize.x) && (abs(yProj) <= halfSize.y);
  }
  static inline vec2f_t
  computeMinMaxVerticesOnAxis(const std::array<vec2f_t, 4> &vertices,
                              const vec2f_t axis) {
    float minProjBox2On1x, maxProjBox2On1x;
    for (size_t i = 0; i < vertices.size(); i++) {
      auto proj = vertices[i].dot(axis);
      if (i == 0) {
        minProjBox2On1x = proj;
        maxProjBox2On1x = proj;
      } else {

        minProjBox2On1x = fmin(minProjBox2On1x, proj);
        maxProjBox2On1x = fmax(maxProjBox2On1x, proj);
      }
    }
    return vec2f_t{.x = minProjBox2On1x, .y = maxProjBox2On1x};
  }
};

struct AxisAlignedBoundingBox {
  vec2f_t position, halfSize;

  inline const std::array<vec2f_t, 4> vertices() const {
    return {
        vec2f_t{.x = position.x - halfSize.x, .y = position.y - halfSize.y},
        vec2f_t{.x = position.x - halfSize.x, .y = position.y + halfSize.y},
        vec2f_t{.x = position.x + halfSize.x, .y = position.y - halfSize.y},
        vec2f_t{.x = position.x + halfSize.x, .y = position.y + halfSize.y},
    };
  }
  inline const bool contains(const vec2f_t &point) {
    auto dist = point.subtract(position);
    return (abs(dist.x) <= halfSize.x) && (abs(dist.y) <= halfSize.y);
  }
};

struct Collider {
  enum ColliderType {
    CIRCLE,
    ORIENTED_BOUNDING_BOX,
    AXIS_ALIGNED_BOUNDING_BOX
  } type;
  union uCollider {
    CircleCollider circle;
    OrientedBoundingBox orientedBoundingBox;
    AxisAlignedBoundingBox axisAlignedBoundingBox;
    uCollider(const CircleCollider &c) : circle(c) {}
    uCollider(const OrientedBoundingBox &obb) : orientedBoundingBox(obb) {}
    uCollider(const AxisAlignedBoundingBox &aabb)
        : axisAlignedBoundingBox(aabb) {}
  } object;

  Collider(const CircleCollider &c) : object(c), type(CIRCLE) {}
  Collider(const OrientedBoundingBox &c)
      : object(c), type(ORIENTED_BOUNDING_BOX) {}
  Collider(const AxisAlignedBoundingBox &c)
      : object(c), type(AXIS_ALIGNED_BOUNDING_BOX) {}

  inline const vec2f_t getPosition() const {
    switch (type) {
    case CIRCLE:
      return object.circle.position;
      break;
    case ORIENTED_BOUNDING_BOX:
      return object.orientedBoundingBox.position;
      break;
    case AXIS_ALIGNED_BOUNDING_BOX:
      return object.axisAlignedBoundingBox.position;
      break;
    }
    return vec2f_t{.x = 0, .y = 0};
  }

  inline void setPosition(const vec2f_t &position) {
    switch (type) {
    case CIRCLE:
      object.circle.position = position;
      break;
    case ORIENTED_BOUNDING_BOX:
      object.orientedBoundingBox.position = position;
      break;
    case AXIS_ALIGNED_BOUNDING_BOX:
      object.axisAlignedBoundingBox.position = position;
      break;
    }
  }

  inline const std::optional<vec2f_t> intersection(const Collider &other) {
    switch (type) {

    case CIRCLE: {

      switch (other.type) {
      case CIRCLE:
        return intersection(object.circle, other.object.circle);
        break;
      case ORIENTED_BOUNDING_BOX:
        return intersection(object.circle, other.object.orientedBoundingBox);
        break;
      case AXIS_ALIGNED_BOUNDING_BOX:
        return intersection(object.circle, other.object.axisAlignedBoundingBox);
        break;
      }
      break;
    }
    case ORIENTED_BOUNDING_BOX: {

      switch (other.type) {
      case CIRCLE:
        return intersection(other.object.circle, object.orientedBoundingBox);
        break;
      case ORIENTED_BOUNDING_BOX:
        return intersection(object.orientedBoundingBox,
                            other.object.orientedBoundingBox);
        break;
      case AXIS_ALIGNED_BOUNDING_BOX:
        return intersection(other.object.axisAlignedBoundingBox,
                            object.orientedBoundingBox);
        break;
      }
      break;
    }
    case AXIS_ALIGNED_BOUNDING_BOX: {

      switch (other.type) {
      case CIRCLE:
        return intersection(other.object.circle, object.axisAlignedBoundingBox);
        break;
      case ORIENTED_BOUNDING_BOX:
        return intersection(object.axisAlignedBoundingBox,
                            other.object.orientedBoundingBox);
        break;
      case AXIS_ALIGNED_BOUNDING_BOX:
        return intersection(object.axisAlignedBoundingBox,
                            other.object.axisAlignedBoundingBox);
        break;
      }
      break;
    }
    }

    return std::nullopt;
  }

  inline const float getArea() {

    switch (type) {

    case CIRCLE: {
      auto r = object.circle.radius;
      return 2.0 * M_PI * (r * r);
      break;
    }
    case ORIENTED_BOUNDING_BOX: {
      auto size = object.orientedBoundingBox.halfSize.scale(2);
      return size.x * size.y;
      break;
    }
    case AXIS_ALIGNED_BOUNDING_BOX: {
      auto size = object.axisAlignedBoundingBox.halfSize.scale(2);
      return size.x * size.y;
      break;
    }
    }
    return 0;
  }

  inline const bool contains(vec2f_t &point) {
    switch (type) {

    case CIRCLE:
      return object.circle.contains(point);
    case ORIENTED_BOUNDING_BOX:
      return object.orientedBoundingBox.contains(point);
    case AXIS_ALIGNED_BOUNDING_BOX:
      return object.axisAlignedBoundingBox.contains(point);
    }
  }

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
  static inline const std::optional<vec2f_t>
  intersection(const CircleCollider &circle1, const CircleCollider &circle2) {

    auto seperationVector = circle1.position.subtract(circle2.position);
    auto distance = seperationVector.length();
    auto sumOfRadii = (circle1.radius + circle2.radius);
    if (distance > sumOfRadii) {
      return std::nullopt;
    }
    auto overlap = distance - sumOfRadii;
    auto minTranslationVector = seperationVector.norm().scale(overlap);
    return minTranslationVector;
  }
  static inline const std::optional<vec2f_t>
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

      auto toEdge = axis.scale(circle.radius);
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

  static inline const std::optional<vec2f_t>
  intersection(const CircleCollider &circle,
               const AxisAlignedBoundingBox &box) {

    auto circleToBox = circle.position.subtract(box.position);
    auto circleToClosestBoxPoint =
        circleToBox.clamp(box.halfSize.scale(-1), box.halfSize);

    auto distanceToBoxSurface = circleToClosestBoxPoint.length();
    if (circle.radius < distanceToBoxSurface) {
      return std::nullopt;
    } else {
      return circleToClosestBoxPoint.norm().scale(circle.radius -
                                                  distanceToBoxSurface);
    }
  }
  static inline const std::optional<vec2f_t>
  intersection(const AxisAlignedBoundingBox &box1,
               const AxisAlignedBoundingBox &box2) {

    auto xProjBox1 = projection_t{.minimum = box1.position.x - box1.halfSize.x,
                                  .maximum = box1.position.x + box1.halfSize.x};

    auto yProjBox1 = projection_t{.minimum = box1.position.y - box1.halfSize.y,
                                  .maximum = box1.position.y + box1.halfSize.y};

    auto xProjBox2 = projection_t{.minimum = box2.position.x - box2.halfSize.x,
                                  .maximum = box2.position.x + box2.halfSize.x};

    auto yProjBox2 = projection_t{.minimum = box2.position.y - box2.halfSize.y,
                                  .maximum = box2.position.y + box2.halfSize.y};

    auto xOverlap = xProjBox1.overlap(xProjBox2);
    if (xOverlap == 0) // seperation was found!
      return std::nullopt;
    auto yOverlap = yProjBox1.overlap(yProjBox2);
    if (yOverlap == 0) // seperation was found!
      return std::nullopt;

    if (xOverlap < yOverlap) {
      return vec2f_t{.x = 1, .y = 0}.scale(xOverlap);
    } else {
      return vec2f_t{.x = 0, .y = 1}.scale(yOverlap);
    }
  }
  static inline const std::optional<vec2f_t>
  intersection(const AxisAlignedBoundingBox &box1,
               const OrientedBoundingBox &box2) {

    auto box1AsOBB =
        OrientedBoundingBox(box1.position, vec2f_t{.x = 1, .y = 0},
                            vec2f_t{.x = 0, .y = 1}, box1.halfSize);
    return intersection(box1AsOBB, box2);
  }
  static inline const std::optional<vec2f_t>
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
};
