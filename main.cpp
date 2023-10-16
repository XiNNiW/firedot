#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_hints.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_sensor.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <algae.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <math.h>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef SDL_AUDIODRIVER
#define SDL_AUDIODRIVER "jack"
#endif // !SDL_AUDIODRIVER

using algae::dsp::_Filter;
using algae::dsp::control::ADEnvelope;
using algae::dsp::filter::Allpass2Comb;
using algae::dsp::filter::InterpolatedDelay;
using algae::dsp::filter::Onepole;
using algae::dsp::filter::ResonantBandpass2ndOrderIIR;
using algae::dsp::filter::SmoothParameter;
using algae::dsp::math::mtof;
using algae::dsp::oscillator::SinOscillator;
using algae::dsp::oscillator::WhiteNoise;
using algae::dsp::spacialization::Pan;

template <typename sample_t> struct Parameter {
  Parameter<sample_t>() { value = 0; }
  Parameter<sample_t>(const sample_t &_value) : value(_value) {}
  Parameter<sample_t>(Parameter<sample_t> &p) { value = p.value; }
  SmoothParameter<sample_t> smoothingFilter;
  std ::atomic<sample_t> value;
  inline const sample_t next() { return smoothingFilter.next(value); }
  void set(sample_t newValue, sample_t smoothingTimeMillis,
           sample_t sampleRate) {
    smoothingFilter.set(smoothingTimeMillis, sampleRate);
    value = newValue;
  }
};

template <typename sample_t> struct DiscreteParameter {
  DiscreteParameter<sample_t>() { value = 0; }
  DiscreteParameter<sample_t>(const sample_t &_value) : value(_value) {}
  DiscreteParameter<sample_t>(DiscreteParameter<sample_t> &p) {
    value = p.value;
  }
  std::atomic<sample_t> value;
  inline const sample_t next() { return value; }
  void set(sample_t newValue) { value = newValue; }
};

template <typename sample_t>
struct Onezero : _Filter<sample_t, Onezero<sample_t>> {
  sample_t x1 = 0;
  sample_t b1 = 0.1;
  const inline sample_t next(const sample_t in) {
    sample_t out = in + b1 * x1;
    x1 = in;
    return out * 0.5;
  }
};

template <typename sample_t> struct KarplusStringVoice {
  static const size_t MAX_DELAY_SAMPS =
      static_cast<size_t>((100.0 * 48000.0) / 1000.0);

  Parameter<sample_t> allpassFilterGain;
  Parameter<sample_t> noteNumber;
  Parameter<sample_t> panPosition;
  Parameter<sample_t> feedback;
  Parameter<sample_t> b1Coefficient;
  Parameter<sample_t> gain;
  Parameter<sample_t> pitchBend;
  DiscreteParameter<sample_t> gate;
  std::atomic<bool> active;

  WhiteNoise<sample_t> exciterNoise;
  SinOscillator<sample_t, sample_t> exciterTone;
  ADEnvelope<sample_t> exciterEnvelope;
  Allpass2Comb<sample_t, MAX_DELAY_SAMPS> apf;
  InterpolatedDelay<sample_t, MAX_DELAY_SAMPS> delay;
  Onepole<sample_t, sample_t> inputFilter;
  Onezero<sample_t> lp;
  Pan<sample_t> panner;

  sample_t samplerate = 48000;
  sample_t y1 = 0;
  sample_t h1 = 1.73;

  KarplusStringVoice<sample_t>() { init(); }
  KarplusStringVoice<sample_t>(const sample_t sr) : samplerate(sr) { init(); }
  void init() {
    inputFilter.lowpass(19000, samplerate);
    exciterEnvelope.set(0.1, 1, samplerate);
    allpassFilterGain.value = -0.5;
    noteNumber.value = 36;
    panPosition.value = 0.5;
    feedback.value = 0.999;
    b1Coefficient.value = 0.999;
    gain.value = 0.8;
  }

  void setSampleRate(sample_t sr) { samplerate = sr; }

  const inline void next(sample_t &leftOut, sample_t &rightOut) {

    const sample_t fb = feedback.next();
    const sample_t b1 = b1Coefficient.next();
    const sample_t apfg = allpassFilterGain.next();
    const sample_t panPos = panPosition.next();
    const sample_t pb = pitchBend.next();
    const sample_t freq = mtof(noteNumber.next() + (12 * pb));
    const sample_t dtime = (samplerate / freq);
    const sample_t allpassDelayTimeSamples = (samplerate / (h1 * freq));
    const sample_t _gain = gain.next();

    sample_t tone = exciterTone.next();
    sample_t noise = exciterNoise.next();

    sample_t exciterLevel = exciterEnvelope.next();
    sample_t exciter = algae::dsp::math::lerp(tone, noise, exciterLevel);
    exciter *= exciterLevel;
    exciter *= _gain;
    exciter = inputFilter.next(exciter + gate.value);
    if (gate.value > 0) {
      exciterEnvelope.trigger();
      gate.set(0);
    }

    sample_t s1 = inputFilter.next(exciter);
    delay.delayTimeSamples = dtime;

    sample_t s2 = delay.next(y1);
    s2 *= fb;
    apf.g = apfg;
    apf.delayTimeSamples = allpassDelayTimeSamples;
    s2 = apf.next(s2);
    lp.b1 = b1;
    s2 = lp.next(s2);

    y1 = s1 + s2;
    panner.setPosition(panPos);
    panner.next(y1 * _gain, leftOut, rightOut);
  }
};

template <typename sample_t> struct NoisePercVoice {
  // graph
  WhiteNoise<sample_t> noise;
  ADEnvelope<sample_t> env;
  ResonantBandpass2ndOrderIIR<sample_t, sample_t> filter;
  Pan<sample_t> panner;
  // parameters
  Parameter<sample_t> filterFreq;
  DiscreteParameter<sample_t> gate;

  sample_t samplerate = 48000;
  NoisePercVoice<sample_t>() { init(); }
  NoisePercVoice<sample_t>(const sample_t sr) : samplerate(sr) { init(); }
  void init() {
    filterFreq.set(800, 30, samplerate);
    env.set(1, 15, samplerate);
    panner.setPosition(0.5);
  }

  void setSampleRate(sample_t sr) { samplerate = sr; }

  void next(sample_t &left, sample_t &right) {
    if (gate.value > 0) {
      env.trigger();
      gate.set(0);
    }
    sample_t out = 0;
    out = noise.next();
    auto e = env.next();
    out *= e * e;
    filter.bandpass(filterFreq.next(), 5, 0.05, samplerate);
    out = filter.next(out);
    panner.next(out, left, right);
  }
};

template <typename sample_t> struct Synthesizer {
private:
  std::array<KarplusStringVoice<sample_t>, 16> strings;
  std::atomic<size_t> stringVoiceIndex = 0;
  std::array<NoisePercVoice<sample_t>, 16> noisePercs;
  std::atomic<size_t> noiseVoiceIndex = 0;
  sample_t sampleRate = 48000;

  inline void setup() {
    for (auto &s : strings) {
      s.setSampleRate(sampleRate);
    }
  }

public:
  Synthesizer<sample_t>(sample_t SR) : sampleRate(SR) { setup(); }
  Synthesizer<sample_t>() { setup(); }

  inline void stringNoteOn(sample_t note) {
    strings[stringVoiceIndex].pitchBend.set(0, 5, sampleRate);
    strings[stringVoiceIndex].noteNumber.set(note, 5, sampleRate);
    strings[stringVoiceIndex].gate.set(1);
    stringVoiceIndex = (stringVoiceIndex + 1) % strings.size();
  }

  inline void noiseNoteOn(sample_t note) {
    noisePercs[noiseVoiceIndex].filterFreq.set(mtof(note), 5, sampleRate);
    noisePercs[noiseVoiceIndex].gate.set(1);
    noiseVoiceIndex = (noiseVoiceIndex + 1) % noisePercs.size();
  }

  inline void setAllpassFilterGain(sample_t g) {
    for (auto &string : strings) {
      string.allpassFilterGain.set(g, 15, sampleRate);
    }
  }

  inline void next(sample_t &leftOut, sample_t &rightOut) {
    leftOut = 0;
    rightOut = 0;
    for (auto &string : strings) {
      sample_t l, r;
      string.next(l, r);
      leftOut += l;
      rightOut += r;
    }
    for (auto &noise : noisePercs) {
      sample_t l, r;
      noise.next(l, r);
      leftOut += l;
      rightOut += r;
    }
  }
};

struct vec2f_t {
  float x = 0, y = 0;
  inline const float length() const { return float(sqrt((x * x) + (y * y))); }
  inline const float dot(const vec2f_t &other) const {
    return (x * other.x) + (y * other.y);
  }
  inline const vec2f_t subtract(const vec2f_t &other) const {
    return vec2f_t{.x = x - other.x, .y = y - other.y};
  }
  inline const vec2f_t add(const vec2f_t &other) const {
    return vec2f_t{.x = x + other.x, .y = y + other.y};
  }
  inline const vec2f_t norm() const {
    auto l = length();
    return vec2f_t{.x = x / l, .y = y / l};
  }
  inline const vec2f_t scale(float factor) const {
    return vec2f_t{.x = x * factor, .y = y * factor};
  }
};

struct CircleCollider {
  vec2f_t position;
  float r;
};

struct OrientedBoundingBox {
  vec2f_t position, axisX, axisY, halfSize;
  OrientedBoundingBox() {}
  OrientedBoundingBox(vec2f_t pos, vec2f_t size, float rotation)
      : position(pos) {
    halfSize = vec2f_t{.x = size.x / float(2.0), .y = size.y / float(2.0)};
    axisX = vec2f_t{.x = cos(rotation), .y = sin(rotation)};
    axisY = vec2f_t{.x = -sin(rotation), .y = cos(rotation)};
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
  inline bool isInBounds(int width, int height) {
    return (position.x <= width) && (position.x >= 0) &&
           (position.y <= height) && (position.y >= 0);
  }
};

struct Particle {
  vec2f_t velocity = vec2f_t{.x = 0, .y = 0};
  int size = 50;
  OrientedBoundingBox collider;

  Particle() {}
  Particle(float x, float y, float size, float rotation = 0) {
    collider = OrientedBoundingBox(vec2f_t{.x = x, .y = y},
                                   vec2f_t{.x = size, .y = size}, rotation);
  }
  inline void update(const float &deltaTimeSeconds,
                     const float &pixelsPerMeter) {
    collider.position = collider.position.add(
        velocity.scale(deltaTimeSeconds).scale(pixelsPerMeter));
  }
};

struct Wall {
  OrientedBoundingBox collider;
  Wall(float x, float y, float w, float h, float rotation = 0)
      : collider(OrientedBoundingBox(vec2f_t{.x = x, .y = y},
                                     vec2f_t{.x = w, .y = h}, rotation)) {}
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
        // minBox1Projection = std::min(minBox1Projection, projection1);
        // maxBox1Projection = std::max(maxBox1Projection, projection1);
        // minBox2Projection = std::min(minBox2Projection, projection2);
        // maxBox2Projection = std::max(maxBox2Projection, projection2);
        box1Projection.minimum = std::min(box1Projection.minimum, projection1);
        box1Projection.maximum = std::max(box1Projection.maximum, projection1);
        box2Projection.minimum = std::min(box2Projection.minimum, projection2);
        box2Projection.maximum = std::max(box2Projection.maximum, projection2);
      }

      auto overlap = box1Projection.overlap(box2Projection);

      //      if ((maxBox2Projection < minBox1Projection) ||
      //          maxBox1Projection < minBox2Projection) {
      //        return std::nullopt;
      //      }

      if (overlap == 0) {
        // no overlap! found seperating axis... these shapes are not touching
        return std::nullopt;
      }
      // auto overlap = abs(std::min(maxBox1Projection, maxBox2Projection) -
      //                    std::max(minBox2Projection, minBox2Projection));

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

      return std::max(float(0), std::min(maximum, other.maximum) -
                                    std::max(minimum, other.minimum));
    }
  };
  std::vector<collision_t> collisions;
  void interact(Particle *p1, Particle *p2,
                const vec2f_t &minTranslationVector) {
    // handle particle particle collision
    // move them outside one another
    auto xi1 = p1->collider.position;
    auto xi2 = p2->collider.position;
    p1->collider.position =
        p1->collider.position.subtract(minTranslationVector);
    // p1->collider.position =
    //     p1->collider.position.subtract(minTranslationVector.scale(0.5));
    // p2->collider.position =
    //     p2->collider.position.add(minTranslationVector.scale(0.5));

    // figure out transfer of momentuum
    const auto density = 1.0;
    double m1 = density * (double(p1->size));
    double m2 = density * (double(p2->size));
    double massSum = m1 + m2;
    auto v1 = p1->velocity;
    auto v2 = p2->velocity;
    auto x1 = p1->collider.position;
    auto x2 = p2->collider.position;
    auto x1_minus_x2 = x1.subtract(x2).scale(1.0 / pixelPerMeter);
    std::stringstream ss;
    ss << "\n";
    ss << "mtv: " << minTranslationVector.x << "," << minTranslationVector.y
       << "\n";
    ss << "p1: " << xi1.x << "," << xi1.y << "\n";
    ss << "p2: " << xi2.x << "," << xi2.y << "\n";

    ss << "v1: " << v1.x << "," << v1.y << "\n";
    ss << "v2: " << v2.x << "," << v2.y << "\n";
    p1->velocity = v1.subtract(x1_minus_x2.scale(
        (2.0 * m2 / massSum) *
        (v1.subtract(v2).dot(x1_minus_x2) / x1_minus_x2.length())));
    auto x2_minus_x1 = x2.subtract(x1).scale(1.0 / pixelPerMeter);
    p2->velocity = v2.subtract(x2_minus_x1.scale(
        (2.0 * m1 / massSum) * v2.subtract(v1).dot(x2_minus_x1) /
        x2_minus_x1.length()));
    ss << "p1: " << x1.x << "," << x1.y << "\n";
    ss << "p2: " << x2.x << "," << x2.y << "\n";
    ss << "v1': " << p1->velocity.x << "," << p1->velocity.y << "\n";
    ss << "v2': " << p2->velocity.x << "," << p2->velocity.y << "\n";
    SDL_Log("%s", ss.str().c_str());
  }
  void interact(Particle *p1, Wall *w2, const vec2f_t &minTranslationVector) {

    //  handle particle wall collision
    //  move it outside the wall
    p1->collider.position =
        p1->collider.position.subtract(minTranslationVector);

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
            auto mvt = intersection(particle1->collider, particle2->collider);
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
            auto mvt = intersection(particle1->collider, wall2->collider);
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
            auto mvt = intersection(particle2->collider, wall1->collider);
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
    // for (auto &c1 : collisions) {
    //   for (std::vector<collision_t>::iterator cit = collisions.begin();
    //        cit != collisions.end();) {
    //     auto c2 = (*cit);
    //     if (((c1.object1 == c2.object1) && (c1.object2 == c2.object2)) ||
    //         ((c1.object1 == c2.object2) && (c1.object2 == c2.object1))) {
    //       cit = collisions.erase(cit);
    //     } else {
    //       ++cit;
    //     }
    //   }
    // }
  }
};

class Framework {
public:
  static void forwardAudioCallback(void *userdata, Uint8 *stream, int len) {
    static_cast<Framework *>(userdata)->audioCallback(stream, len);
  }
  Framework(int height_, int width_) : height(height_), width(width_) {}

  inline const bool init() {

    if (!loadConfig()) {
      SDL_LogError(0, "unable to load config!");
      return false;
    }

    SDL_Init(SDL_INIT_VIDEO); // Initializing SDL as Video
    SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_OPENGL, &window,
                                &renderer); // Get window surface

    if (window == NULL) {
      printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
      return false;
    }

    // Initialize PNG loading
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
      printf("SDL_image could not initialize! SDL_image Error: %s\n",
             IMG_GetError());
      return false;
    }

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
      printf("SDL_joystick could not initialize! Error: %s\n", SDL_GetError());
    // Check for joysticks
    if (SDL_NumJoysticks() < 1) {
      printf("Warning: No joysticks connected!\n");
    } else {
      // Load joystick
      gGameController = SDL_JoystickOpen(0);
      if (gGameController == NULL) {
        printf("Warning: Unable to open game controller! SDL Error: %s\n",
               SDL_GetError());
      }
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
      printf("SDL_audio could not initialize! Error: %s\n", SDL_GetError());

    auto numAudioDrivers = SDL_GetNumAudioDrivers();
    for (int i = 0; i < numAudioDrivers; i++) {
      SDL_LogInfo(0, "Driver# %d: %s\n", i, SDL_GetAudioDriver(i));
    }
    SDL_LogInfo(0, "active driver: %s\n", SDL_GetCurrentAudioDriver());

    const auto desiredAudioConfig = SDL_AudioSpec{
        .freq = static_cast<int>(SAMPLE_RATE),
        .format = AUDIO_F32,
        .channels = static_cast<Uint8>(NUM_CHANNELS),
        .samples = static_cast<Uint16>(BUFFER_SIZE),
        .callback = Framework::forwardAudioCallback,
        .userdata = this,
    };

    audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &desiredAudioConfig, &config,
                                        SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (audioDeviceID == 0) {
      SDL_LogError(0, "Couldn't open audio: %s\n", SDL_GetError());
      return false;
    } else {
      SDL_LogInfo(0, "Audio status for device# %d: %s\n", audioDeviceID,
                  SDL_GetAudioDeviceName(audioDeviceID, 0));
    }
    SDL_PauseAudioDevice(audioDeviceID, 0); // start playback

    SDL_SetRenderDrawColor(renderer, 0, 0, 0,
                           0);   // setting draw color
    SDL_RenderClear(renderer);   // Clear the newly created window
    SDL_RenderPresent(renderer); // Reflects the changes done in the
    //  window.
    if (!loadMedia()) {
      printf("Media could not be loaded...\n");
    }

    if (std::atomic<float>{}.is_lock_free()) {
      SDL_LogInfo(0, "atomic is lock free!");
    } else {
      SDL_LogError(0, "no hardware atomic... using mutex fallback!");
    }

    addWalls();
    physics.gravity = vec2f_t{.x = 4, .y = 9.8};
    lastFrameTime = SDL_GetTicks();
    // init was successful
    return true;
  }

  ~Framework() {

    for (auto o : gameObjects) {
      delete o;
    }
    gameObjects.clear();

    SDL_JoystickClose(gGameController);
    gGameController = NULL;
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_DestroyTexture(gCursor);
    gCursor = NULL;
    IMG_Quit();
    SDL_Quit();
  }

  void audioCallback(Uint8 *stream, int len) {
    /* 2 channels, 4 bytes/sample = 8 bytes/frame */
    float *sampleStream = (float *)stream;
    size_t numRequestedSamples = len / (config.channels * 4);
    for (size_t i = 0; i < numRequestedSamples; i++) {

      synth.next(sampleStream[i * config.channels],
                 sampleStream[i * config.channels + 1]);
    }
  };

  void addWalls() {
    gameObjects.push_back(
        new GameObject(Wall(0.0, height / 2.0, 50, height / 2.0)));

    gameObjects.push_back(
        new GameObject(Wall(width, height / 2.0, 50, height / 2.0)));

    gameObjects.push_back(
        new GameObject(Wall(width / 2.0, 0, width / 2.0, 50)));

    gameObjects.push_back(
        new GameObject(Wall(width / 2.0, height, width / 2.0, 50)));
  }

  bool loadConfig() {
    // perhaps this will be where one could load color schemes or other config
    // that is saved between runs of the app

    if (std::string(SDL_AUDIODRIVER).compare("jack") == 0) {

      putenv((char *)"SDL_AUDIODRIVER=jack");
    } else {

      putenv((char *)"SDL_AUDIODRIVER=openslES");
    }
    return true;
  }

  bool loadMedia() {
    bool success = true;

    auto path = "images/p04_shape1.bmp";
    SDL_Surface *surface = IMG_Load(path);
    gCursor = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (gCursor == NULL) {
      printf("Unable to load image %s! SDL Error: %s\n", path, SDL_GetError());
      success = false;
    }

    return success;
  }

  void update(SDL_Event &event) {
    handleEvent(event);
    updatePhysics(event);
    evaluateGameRules(event);
  }

  void handleEvent(SDL_Event &event) {
    // frame rate sync
    auto deltaTimeMilliseconds = SDL_GetTicks() - lastFrameTime;
    auto timeToWait = FRAME_DELTA_MILLIS - deltaTimeMilliseconds;
    frameDeltaTimeSeconds = deltaTimeMilliseconds / 1000.0;
    if ((timeToWait > 0) && (timeToWait < FRAME_DELTA_MILLIS))
      SDL_Delay(timeToWait);
    lastFrameTime = SDL_GetTicks();
    // Event loop
    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
      case SDL_QUIT:
        return;
      case SDL_APP_WILLENTERBACKGROUND:
        SDL_PauseAudioDevice(audioDeviceID, 1);
        renderIsOn = false;
        SDL_Log("Entering background");
        break;
      case SDL_APP_DIDENTERFOREGROUND:
        SDL_PauseAudioDevice(audioDeviceID, 0);
        renderIsOn = true;
        SDL_Log("Entering foreground");
        break;
      case SDL_RENDER_DEVICE_RESET:
        SDL_LogWarn(0, "Render device reset!");
        break;
      case SDL_RENDER_TARGETS_RESET:
        SDL_LogWarn(0, "Render targets reset!");
        break;
      case SDL_MOUSEMOTION:
        mousePositionX = event.motion.x;
        mousePositionY = event.motion.y;
        break;
      case SDL_MOUSEBUTTONDOWN: {
        mouseDownPositionX = event.motion.x;
        mouseDownPositionY = event.motion.y;
        // particles.push_back(
        //     Particle(mouseDownPositionX, mouseDownPositionY, 50, 0));
        // int note = std::floor((float(event.motion.y) / float(width)) * 24
        // + 36); synth.noteOn(note);
        break;
      }
      case SDL_MOUSEBUTTONUP: {

        auto mDown = vec2f_t{.x = float(mouseDownPositionX),
                             .y = float(mouseDownPositionY)};
        auto mUp =
            vec2f_t{.x = float(event.motion.x), .y = float(event.motion.y)};

        auto particleSize = mUp.subtract(mDown).length();
        gameObjects.push_back(
            new GameObject(Particle(mDown.x, mDown.y, particleSize)));

        break;
      }
      case SDL_MULTIGESTURE: {
        // float pinchDistance = event.mgesture.dDist;
        // auto x = event.mgesture.x;
        // auto y = event.mgesture.y;
        // auto dx = mousePositionX - x;
        // auto dy = mousePositionY - y;
        // radius = sqrt((dx * dx) + (dy * dy));
        // synth.pitchBend(float(radius) / float(width));
        break;
      }
      case SDL_JOYAXISMOTION: {
        // X axis motion
        float xDir = 0;
        float yDir = 0;
        if (event.jaxis.axis == 0) {
          joystickXPosition = event.jaxis.value;
          // Left of dead zone
          if (event.jaxis.value < -JOYSTICK_DEAD_ZONE) {
            xDir = -1;
          }
          // Right of dead zone
          else if (event.jaxis.value > JOYSTICK_DEAD_ZONE) {
            xDir = 1;
          } else {
            xDir = 0;
          }
        }
        // Y Axis motion
        else if (event.jaxis.axis == 1) {
          joystickYPosition = event.jaxis.value;
          // Below of dead zone
          if (event.jaxis.value < -JOYSTICK_DEAD_ZONE) {
            yDir = -1;
          }
          // Above of dead zone
          else if (event.jaxis.value > JOYSTICK_DEAD_ZONE) {
            yDir = 1;
          } else {
            yDir = 0;
          }
        }
        // Calculate angle
        double joystickAngle =
            atan2((double)yDir, (double)xDir) * (180.0 / M_PI);

        // Correct angle
        if (xDir == 0 && yDir == 0) {
          joystickAngle = 0;
        }

        float jx = float(xDir * joystickYPosition) / float(JOYSTICK_MAX_VALUE);
        float jy = float(yDir * joystickXPosition) / float(JOYSTICK_MAX_VALUE);

        float g = 9.8;
        physics.gravity = vec2f_t{.x = g * jx, .y = g * jy};

        break;
      }
      case SDL_SENSORUPDATE: {
        // float data[6];
        // SDL_SensorGetData()
        std::ostringstream ss;
        auto sensor = SDL_SensorFromInstanceID(event.sensor.which);
        auto sensorName = SDL_SensorGetName(sensor);
        ss << "sensor: " << sensorName << " " << event.sensor.data;
        std::string message = ss.str();
        SDL_Log("%s", message.c_str());
        break;
      }
      }
    }
  }

  void updatePhysics(SDL_Event &event) {
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;
    physics.update(frameDeltaTimeSeconds, &gameObjects);
  }

  void evaluateGameRules(SDL_Event &event) {
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;

    for (auto &collision : physics.getCollisions()) {
      auto obj1 = collision.object1;
      auto obj2 = collision.object2;

      switch (obj1->type) {
      case GameObject::PARTICLE: {
        switch (obj2->type) {
        case GameObject::PARTICLE: {
          auto p1 = obj1->object.particle;
          auto p2 = obj2->object.particle;
          if (p2.velocity.subtract(p1.velocity).length() > 1)
            synth.noiseNoteOn(72);
          break;
        }
        case GameObject::WALL: {
          auto w1 = obj1->object.wall;
          auto p2 = obj1->object.particle;
          if (p2.velocity.length() > 1) {
            synth.stringNoteOn(60);
          }
          break;
        }
        }
        break;
      }
      case GameObject::WALL: {
        switch (obj2->type) {
        case GameObject::PARTICLE: {

          auto p2 = obj1->object.particle;
          if (p2.velocity.length() > 1) {
            synth.stringNoteOn(60);
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

    for (std::vector<GameObject *>::iterator objectIterator =
             gameObjects.begin();
         objectIterator != gameObjects.end();) {
      switch ((*objectIterator)->type) {
      case GameObject::PARTICLE: {
        if (!(*objectIterator)
                 ->object.particle.collider.isInBounds(width, height)) {
          objectIterator = gameObjects.erase(objectIterator);
        } else {
          ++objectIterator;
        }
        break;
      }
      case GameObject::WALL:
        ++objectIterator;
        break;
      }
    }
  }

  void draw(SDL_Event &event) {
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;
    SDL_RenderClear(renderer);
    // drawing code here

    // SDL_SetTextureColorMod(gHelloWorld, 255, 255, 0);
    // SDL_RenderCopy(renderer, gCursor, NULL, &dst);

    for (auto object : gameObjects) {

      switch (object->type) {
      case GameObject::PARTICLE: {
        auto particle = object->object.particle;
        auto destRect = SDL_Rect();
        destRect.x =
            particle.collider.position.x - particle.collider.halfSize.x;
        destRect.y =
            particle.collider.position.y - particle.collider.halfSize.y;
        destRect.w = 2 * particle.collider.halfSize.x;
        destRect.h = 2 * particle.collider.halfSize.y;
        double angle =
            atan(particle.collider.axisX.y / particle.collider.axisX.x);
        SDL_Point center = SDL_Point();
        center.x = particle.collider.position.x;
        center.y = particle.collider.position.y;
        SDL_SetTextureColorMod(gCursor, 255, 255, 0);
        SDL_RenderCopyEx(renderer, gCursor, NULL, &destRect, angle, &center,
                         SDL_FLIP_NONE);
        break;
      }
      case GameObject::WALL: {
        auto wall = object->object.wall;
        auto destRect = SDL_Rect();
        destRect.x = wall.collider.position.x - wall.collider.halfSize.x;
        destRect.y = wall.collider.position.y - wall.collider.halfSize.y;
        destRect.w = 2 * wall.collider.halfSize.x;
        destRect.h = 2 * wall.collider.halfSize.y;
        double angle = atan(wall.collider.axisX.y / wall.collider.axisX.x);
        SDL_Point center = SDL_Point();
        center.x = wall.collider.position.x;
        center.y = wall.collider.position.y;
        SDL_SetTextureColorMod(gCursor, 0, 255, 255);
        SDL_RenderCopyEx(renderer, gCursor, NULL, &destRect, angle, &center,
                         SDL_FLIP_NONE);
        break;
      }
      }
    }

    SDL_RenderPresent(renderer);
  }

private:
  const int FRAME_RATE_TARGET = 120;
  const int FRAME_DELTA_MILLIS = (1.0 / float(FRAME_RATE_TARGET)) * 1000.0;
  const size_t BUFFER_SIZE = 128;
  const float SAMPLE_RATE = 48000;
  const size_t NUM_CHANNELS = 2;
  const int JOYSTICK_DEAD_ZONE = 8000;
  const int JOYSTICK_MAX_VALUE = 32767;
  const int JOYSTICK_MIN_VALUE = -32768;
  int height;                    // Height of the window
  int width;                     // Width of the window
  SDL_Renderer *renderer = NULL; // Pointer for the renderer
  SDL_Window *window = NULL;     // Pointer for the window
  SDL_Texture *gCursor = NULL;
  SDL_Joystick *gGameController = NULL;
  std::vector<GameObject *> gameObjects;
  int mousePositionX = 0;
  int mousePositionY = 0;
  int mouseDownPositionX = 0;
  int mouseDownPositionY = 0;
  int joystickXPosition = 0;
  int joystickYPosition = 0;
  SDL_AudioSpec config;
  Synthesizer<float> synth;
  int lastFrameTime = 0;
  int radius = 50;
  bool renderIsOn = true;
  int audioDeviceID = -1;
  Physics physics;
  double frameDeltaTimeSeconds;
};

int main(int argc, char *argv[]) {
  Framework game(2220, 940);
  // only proceed if init was success
  if (game.init()) {

    SDL_Event event; // Event variable

    while (!(event.type == SDL_QUIT)) {
      // SDL_Log("tick\n");
      game.update(event);
      game.draw(event);
    }
  } else {
    SDL_LogError(0, "Initialization of game failed: %s", SDL_GetError());
  }

  return 0;
}
