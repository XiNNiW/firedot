#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_hints.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_sensor.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#include "include/collider.h"
#include "include/game_object.h"
#include "include/metaphor.h"
#include "include/physics.h"
#include "include/save_state.h"
#include "include/sensor.h"
#include "include/sequencer.h"
#include "include/synthesis.h"
#include "include/synthesis_parameter.h"
#include "include/ui.h"
#include "include/vector_math.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
#include <algae.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
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

// enum class GameState { RUNNING, WAITING_FOR_FRAME_SYNC, PAUSED };

static inline const bool LoadIconTexture(SDL_Renderer *renderer,
                                         std::string path,
                                         SDL_Texture *texture) {

  SDL_Surface *surface = IMG_Load(path.c_str());
  texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  if (texture == NULL) {
    SDL_LogError(0,
                 "Unable to load image %s! "
                 "SDL Error: %s\n",
                 path.c_str(), SDL_GetError());
    return false;
  } else {
    return true;
  }
}

inline void FreeSampleInfo(AudioSample *sampleInfo) {
  // free(sampleInfo->buffer);
  // free(sampleInfo);
  delete sampleInfo;
}

static inline bool LoadWAVSampleAsMono(std::string samplePath,
                                       AudioSample **sampleInfo) {

  SDL_AudioSpec wavSpec;
  Uint32 wavLength;
  Uint8 *wavBuffer;
  long bufferSize = 0;

  SDL_LoadWAV(samplePath.c_str(), &wavSpec, &wavBuffer, &wavLength);

  bool isSupportedFormat = false;

  switch (wavSpec.format) {
  case AUDIO_S8: {
    const int bytesPerSample = sizeof(int8_t);
    SDL_Log("its a signed 8bit int!");
    break;
  }
  case AUDIO_U8: {
    const int bytesPerSample = sizeof(uint8_t);
    SDL_Log("its a unsigned 8bit int!");
    break;
  }
  case AUDIO_S16: {
    const int bytesPerSample = sizeof(int16_t);
    isSupportedFormat = true;
    SDL_Log("its a signed 16bit int!");
    int16_t *stream = (int16_t *)wavBuffer;

    SDL_Log("sample info: %d %f %d - %f", wavLength,
            wavLength / float(bytesPerSample), wavSpec.freq,
            float(wavLength / float(bytesPerSample * wavSpec.channels)) /
                float(wavSpec.freq));
    bufferSize = wavLength / bytesPerSample;
    *sampleInfo = new AudioSample(wavSpec.freq, wavSpec.channels, bufferSize);
    if (*sampleInfo == NULL) {
      SDL_LogError(0, "sample failed to init!");
    }
    if ((*sampleInfo)->buffer == NULL) {
      return false;
    }
    for (size_t i = 0; i < bufferSize; ++i) {
      (*sampleInfo)->buffer[i] = 0;
      for (size_t ch = 0; ch < wavSpec.channels; ++ch) {
        (*sampleInfo)->buffer[i] +=
            float(stream[i + ch]) / float(std::numeric_limits<int16_t>::max());
      }
    }

    break;
  }
  case AUDIO_U16: {
    const int bytesPerSample = sizeof(uint16_t);
    SDL_Log("its a unsigned 16 bit int!");
    break;
  }
  case AUDIO_S32: {
    const int bytesPerSample = sizeof(int32_t);
    SDL_Log("its a signed 32 bit int!");
    break;
  }
  case AUDIO_F32: {
    const int bytesPerSample = sizeof(float);
    isSupportedFormat = true;
    SDL_Log("its a float!");
    break;
  }
  default:
    SDL_Log("could not detect datatype");
  }

  free(wavBuffer);
  if ((*sampleInfo) == NULL) {

    SDL_LogError(0, "freeing wav buffer killed sample info!");
  }
  SDL_Log("loaded sample!");
  return isSupportedFormat;
}

class Framework {
public:
  static inline void forwardAudioCallback(void *userdata, Uint8 *stream,
                                          int len) {
    static_cast<Framework *>(userdata)->audioCallback(stream, len);
  }
  Framework(int width_, int height_)
      : height(height_), width(width_),
        synth(Synthesizer<float>(SubtractiveSynthesizer<float>())) {}

  inline const bool init() {

    std::stringstream ss;
    ss << "initial size: " << width << ", " << height << "\n";
    if (!loadConfig()) {
      SDL_LogError(0, "unable to load config!");
      return false;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      SDL_LogError(0, "could not init! %s", SDL_GetError());
      return false;
    } // Initializing SDL as Video
    // | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP |
    // SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_ALLOW_HIGHDPI
    if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_OPENGL, &window,
                                    &renderer) < 0) {
      SDL_LogError(0, "could not get window! %s", SDL_GetError());
      return false;
    } // Get window surface

    if (window == NULL) {
      printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
      return false;
    }
    SDL_GetWindowSize(window, &width, &height);
    // SDL_GL_GetDrawableSize(window,&width, &height);
    ss << "adjusted size: " << width << ", " << height << "\n";
    // SDL_RenderSetLogicalSize(renderer, width, height);
    //  SDL_SetWindowSize(&window, width, height);

    // Initialize PNG loading
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
      printf("SDL_image could not initialize! SDL_image Error: %s\n",
             IMG_GetError());
      return false;
    }

    if (SDL_InitSubSystem(SDL_INIT_SENSOR) < 0) {
      SDL_LogError(0, "SDL_sensor could not initialize! Error: %s\n",
                   SDL_GetError());
    }

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
      SDL_LogError(0, "SDL_joystick could not initialize! Error: %s\n",
                   SDL_GetError());
    // Check for joysticks
    if (SDL_NumJoysticks() < 1) {
      SDL_LogWarn(0, "Warning: No joysticks connected!\n");
    } else {
      // Load joystick
      gGameController = SDL_JoystickOpen(0);
      if (gGameController == NULL) {
        SDL_LogWarn(0,
                    "Warning: Unable to open game controller! SDL Error: %s\n",
                    SDL_GetError());
      }
    }
    SDL_LogInfo(0, "num sensors: %d", SDL_NumSensors());
    for (size_t i = 0; i < SDL_NumSensors(); i++) {
      auto sensor = SDL_SensorOpen(i);
      if (sensor == NULL) {
        SDL_LogError(0, "%s", SDL_GetError());
      } else {
        SDL_LogInfo(0, " %s", SDL_SensorGetName(sensor));
      }
    }
    if (TTF_Init() < 0) {
      SDL_LogError(0, "SDL_ttf failed to init %s\n", SDL_GetError());
      return false;
    }

    auto font = TTF_OpenFont("fonts/Roboto-Medium.ttf", 36);
    if (font == NULL) {
      SDL_LogError(0, "failed to load font: %s\n", SDL_GetError());
      return false;
    }

    style = Style{
        .font = font,
    };

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
      SDL_LogError(0, "SDL_audio could not initialize! Error: %s\n",
                   SDL_GetError());

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
    // audioDeviceID =
    //    SDL_OpenAudioDevice(NULL, 0, &desiredAudioConfig, &config, 0);
    if (audioDeviceID == 0) {
      SDL_LogError(0, "Couldn't open audio: %s\n", SDL_GetError());
      return false;
    } else {
      SDL_LogInfo(0,
                  "Audio status for "
                  "device# %d: %s\n",
                  audioDeviceID, SDL_GetAudioDeviceName(audioDeviceID, 0));
    }

    // init was successful

    ss << "audio conf: \n";
    ss << "format: " << config.format << "\n";
    ss << "channels: " << config.channels << "\n";
    ss << "freq: " << config.freq << "\n";
    ss << "padding: " << config.padding << "\n";
    ss << "size: " << config.size << "\n";
    ss << "silence: " << config.silence << "\n";

    if (!LoadWAVSampleAsMono("sounds/autoharp 13 C3.wav", &audioSample)) {
      SDL_LogError(0, "could not read sample!");
      return false;
    };
    if (audioSample == NULL) {
      SDL_LogError(0, "sampleLoad failed!");
      return false;
    }

    SDL_LogInfo(0, "%s", ss.str().c_str());

    if (!loadMedia()) {
      SDL_LogError(0, "Media could not be "
                      "loaded...\n");
    }

    if (std::atomic<float>{}.is_lock_free()) {
      SDL_LogInfo(0, "atomic is lock free!");
    } else {
      SDL_LogError(0, "no hardware atomic... "
                      "using mutex fallback!");
    }

    synth.setSynthType(SUBTRACTIVE_DRUM_SYNTH);
    synth.setGain(1);
    synth.setFilterCutoff(1);
    synth.setFilterQuality(0.5);
    synth.setSoundSource(0.5);
    synth.setAttackTime(0.001);
    synth.setReleaseTime(1);
    synth.activeSample = audioSample;

    userInterface.buildLayout(
        {.position = {.x = static_cast<float>(width / 2.0),
                      .y = static_cast<float>(height / 2.0)},
         .halfSize = {.x = static_cast<float>(width / 2.0),
                      .y = static_cast<float>(height / 2.0)}});

    sensorMapping.addMapping(TILT, ContinuousParameterType::SOUND_SOURCE);
    sensorMapping.addMapping(INSTRUMENT_GATE, MomentaryParameterType::GATE);
    sensorMapping.addMapping(KEYBOARD_KEY, ContinuousParameterType::FREQUENCY);
    sensorMapping.addMapping(ACCELERATION,
                             ContinuousParameterType::FILTER_CUTOFF);

    SDL_PauseAudioDevice(audioDeviceID, 0); // start playback

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // setting draw color
    SDL_RenderClear(renderer);                    // Clear the newly
                                                  // created window
    SDL_RenderPresent(renderer);                  // Reflects the
                                                  // changes done in the window.

    lastFrameTime = SDL_GetTicks();

    return true;
  }

  ~Framework() {
    for (auto o : gameObjects) {
      delete o;
    }
    gameObjects.clear();

    FreeSampleInfo(audioSample);

    SDL_JoystickClose(gGameController);
    gGameController = NULL;
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    SDL_DestroyTexture(gCursor);
    gCursor = NULL;
    SDL_DestroyTexture(menuIcon);
    menuIcon = NULL;
    SDL_DestroyTexture(synthSelectIcon);
    synthSelectIcon = NULL;
    TTF_CloseFont(style.font);
    style.font = NULL;
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
  }

  void audioCallback(Uint8 *stream, int numBytesRequested) {
    memset(stream, config.silence, numBytesRequested);
    /* 2 channels, 4 bytes/sample = 8
     * bytes/frame */
    const int bytesPerSample = sizeof(float);
    float *sampleStream = (float *)stream;
    size_t numRequestedSamplesPerChannel =
        numBytesRequested / (config.channels * bytesPerSample);
    const size_t fixedBlockSize = 64;
    const size_t numBlocks = numRequestedSamplesPerChannel / fixedBlockSize;
    const size_t remainingSamples =
        numRequestedSamplesPerChannel % fixedBlockSize;

    float *ch1Pointer = &sampleStream[0];
    float *ch2Pointer = &sampleStream[1];

    for (size_t i = 0; i < numBlocks; ++i) {
      float block[fixedBlockSize];
      synth.process(block, fixedBlockSize);
      for (size_t j = 0; j < fixedBlockSize; ++j) {
        auto nextSample = block[j];
        *ch1Pointer = nextSample;
        *ch2Pointer = nextSample;
        ch1Pointer += config.channels;
        ch2Pointer += config.channels;
      }
    }

    for (size_t i = 0; i < remainingSamples; ++i) {
      // for (size_t i = 0; i < numRequestedSamplesPerChannel; ++i) {
      auto nextSample = synth.next();
      *ch1Pointer = nextSample;
      *ch2Pointer = nextSample;
      ch1Pointer += config.channels;
      ch2Pointer += config.channels;
    }
  };

  bool loadConfig() {
    // perhaps this will be where one
    // could load color schemes or other
    // config that is saved between runs
    // of the app

    if (std::string(SDL_AUDIODRIVER).compare("jack") == 0) {

      putenv((char *)"SDL_AUDIODRIVER=jack");
    } else {

      putenv((char *)"SDL_AUDIODRIVER="
                     "openslES");
    }
    return true;
  }

  bool loadMedia() {

    if (!LoadIconTexture(renderer, "images/p04_shape1.bmp", gCursor)) {
      return false;
    }

    if (!LoadIconTexture(renderer, "images/menu-burger-svgrepo-com.svg",
                         menuIcon)) {
      return false;
    }

    if (!LoadIconTexture(renderer, "images/colour-tuneing-svgrepo-com.svg",
                         synthSelectIcon)) {
      return false;
    }

    return true;
  }

  void update(SDL_Event &event) {
    // frame rate sync
    auto deltaTimeMilliseconds = SDL_GetTicks() - lastFrameTime;
    auto timeToWait = FRAME_DELTA_MILLIS - deltaTimeMilliseconds;
    frameDeltaTimeSeconds = deltaTimeMilliseconds / 1000.0;
    if ((timeToWait > 0) && (timeToWait < FRAME_DELTA_MILLIS)) {
      SDL_Delay(timeToWait);
    }
    lastFrameTime = SDL_GetTicks();

    if (saveState.instrumentMetaphor == InstrumentMetaphorType::SEQUENCER) {
      sequencer.update(frameDeltaTimeSeconds);
    }
    handleEvents(event);
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;

    updatePhysics(event);

    evaluateGameRules(event);
  }

  void handleEvents(SDL_Event &event) {

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
        mousePosition.x = event.motion.x;
        mousePosition.y = event.motion.y;
        userInterface.handleMouseMove(mousePosition);
        break;
      case SDL_MOUSEBUTTONDOWN: {
        mouseDownPosition.x = event.motion.x;
        mouseDownPosition.y = event.motion.y;

        userInterface.handleMouseDown(mousePosition);

        break;
      }
      case SDL_MOUSEBUTTONUP: {

        userInterface.handleMouseUp(mousePosition);

        break;
      }
      case SDL_MULTIGESTURE: {

        break;
      }
      case SDL_FINGERMOTION: {
        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};
        auto pressure = event.tfinger.pressure;

        userInterface.handleFingerMove(fingerId, position, pressure);
        break;
      }
      case SDL_FINGERDOWN: {
        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};

        auto pressure = event.tfinger.pressure;

        userInterface.handleFingerDown(fingerId, position, pressure);
        break;
      }
      case SDL_FINGERUP: {

        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};

        auto pressure = event.tfinger.pressure;

        userInterface.handleFingerUp(fingerId, position, pressure);

        break;
      }
      case SDL_JOYAXISMOTION: {
        break;
      }
      case SDL_SENSORUPDATE: {

        std::stringstream ss;
        auto sensor = SDL_SensorFromInstanceID(event.sensor.which);
        auto sensorName = SDL_SensorGetName(sensor);
        auto sensorType = SDL_SensorGetType(sensor);
        switch (sensorType) {
        case SDL_SENSOR_ACCEL: {
          ss << "sensor: " << sensorName << " " << event.sensor.data[0] << ", "
             << event.sensor.data[1] << ", " << event.sensor.data[2];

          physics.gravity = vec2f_t{.x = event.sensor.data[0] * -3,
                                    .y = event.sensor.data[1] * 3};
          auto acc3 = vec3f_t{.x = event.sensor.data[0] * -1,
                              .y = event.sensor.data[1],
                              .z = event.sensor.data[2]}
                          .scale(1.0 / 9.8);

          auto accVector = vec2f_t{.x = acc3.x, .y = acc3.y};

          // synth.setSoundSource(accVector.length());
          sensorMapping.emitEvent(&synth, ContinuousInputType::TILT,
                                  accVector.length());

          break;
        }
        case SDL_SENSOR_GYRO: {
          ss << "sensor: " << sensorName << " " << event.sensor.data[0] << ", "
             << event.sensor.data[1] << ", " << event.sensor.data[2];
          auto velocity = vec3f_t{.x = event.sensor.data[0],
                                  .y = event.sensor.data[1],
                                  .z = event.sensor.data[2]};
          sensorMapping.emitEvent(&synth, ContinuousInputType::ACCELERATION,
                                  clip<float>(velocity.length()));
          // SDL_LogInfo(0, "%s", ss.str().c_str());
          // sensorMapping.emitEvent(&synth, ContinuousInputType::ROLL,
          //                        fmax(0, fmin(1,
          //                        abs(event.sensor.data[0]))));
          // sensorMapping.emitEvent(&synth, ContinuousInputType::PITCH,
          //                        fmax(0, fmin(1,
          //                        abs(event.sensor.data[1]))));
          // sensorMapping.emitEvent(&synth, ContinuousInputType::YAW,
          //                        fmax(0, fmin(1,
          //                        abs(event.sensor.data[2]))));
          break;
        }
        default:
          break;
        }
        break;
      }
      }
    }

    // aoshd
  }

  void updatePhysics(SDL_Event &event) {
    physics.update(frameDeltaTimeSeconds, &gameObjects);
  }

  void evaluateGameRules(SDL_Event &event) {}

  void draw(SDL_Event &event) {
    // drawing code here
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;
    SDL_RenderClear(renderer);

    for (auto object : gameObjects) {
    }

    userInterface.draw(renderer, style);
    SDL_RenderPresent(renderer);
  }

private:
  const int FRAME_RATE_TARGET = 120;
  const int FRAME_DELTA_MILLIS = (1.0 / float(FRAME_RATE_TARGET)) * 1000.0;
  const size_t BUFFER_SIZE = 256;
  const float SAMPLE_RATE = 48000;
  const size_t NUM_CHANNELS = 2;
  const int JOYSTICK_DEAD_ZONE = 3000; // 8000;
  const int JOYSTICK_MAX_VALUE = 32767;
  const int JOYSTICK_MIN_VALUE = -32768;
  int height;                    // Height of the window
  int width;                     // Width of the window
  SDL_Renderer *renderer = NULL; // Pointer for the renderer
  SDL_Window *window = NULL;     // Pointer for the window
  SDL_Texture *gCursor = NULL;
  SDL_Joystick *gGameController = NULL;
  // GameState gameState = GameState::RUNNING;
  std::vector<GameObject *> gameObjects;
  GameObject *wall1, *wall2, *wall3, *wall4;
  AudioSample *audioSample = NULL;

  Style style;
  SDL_Texture *menuIcon = NULL;
  SDL_Texture *synthSelectIcon = NULL;

  // Model objects
  Synthesizer<float> synth;
  InputMapping<float> sensorMapping;
  Sequencer sequencer = Sequencer(&synth);

  SaveState saveState;

  // UI objects
  UserInterface userInterface =
      UserInterface(&synth, &sensorMapping, &sequencer, &saveState);

  SDL_Color textColor = {20, 20, 20};
  SDL_Color textBackgroundColor = {0, 0, 0};
  double lastAccZ = 9.8;
  vec2f_t mousePosition = vec2f_t{0, 0};
  vec2f_t mouseDownPosition = vec2f_t{0, 0};
  bool mouseIsDown = false;
  std::map<SDL_FingerID, int> heldKeys;
  int joystickXPosition = 0;
  int joystickYPosition = 0;
  float xDir = 0, yDir = 0;
  SDL_AudioSpec config;

  int lastFrameTime = 0;
  int radius = 50;
  bool renderIsOn = true;
  int audioDeviceID = -1;
  double previousAccelerationZ = 9.8;
  Physics physics;
  double frameDeltaTimeSeconds;
};

int main(int argc, char *argv[]) {
  SDL_Log("begin!");
  // Framework game(1920, 1080);
  Framework game(940, 2220);
  // only proceed if init was success
  if (game.init()) {

    SDL_Event event; // Event variable

    while (event.type != SDL_QUIT) {

      game.update(event);
      game.draw(event);
    }
  } else {
    SDL_LogError(0,
                 "Initialization of "
                 "game failed: %s",
                 SDL_GetError());
  }

  return 0;
}
