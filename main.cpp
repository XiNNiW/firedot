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
#include "include/game_object.h"
#include "include/physics.h"
#include "include/synthesis.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
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
#include <sstream>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef SDL_AUDIODRIVER
#define SDL_AUDIODRIVER "jack"
#endif // !SDL_AUDIODRIVER

class Framework {
public:
  static void forwardAudioCallback(void *userdata, Uint8 *stream, int len) {
    static_cast<Framework *>(userdata)->audioCallback(stream, len);
  }
  Framework(int height_, int width_)
      : height(height_), width(width_),
        synth(Synthesizer<float>(SubtractiveSynthesizer<float>())) {
    ;
  }

  inline const bool init() {

    if (!loadConfig()) {
      SDL_LogError(0, "unable to load config!");
      return false;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      SDL_LogError(0, "could not init! %s", SDL_GetError());
      return false;
    } // Initializing SDL as Video
    if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_OPENGL, &window,
                                    &renderer) < 0) {
      SDL_LogError(0, "could not get window! %s", SDL_GetError());
      return false;
    } // Get window surface

    SDL_GetWindowSize(window, &width, &height);

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
    //    if (TTF_Init() < 0) {
    //      SDL_LogError(0, "SDL_ttf failed to init %s\n", SDL_GetError());
    //      return false;
    //    }

    //    font = TTF_OpenFont("fonts/Roboto-Medium.ttf", 16);
    //    if (font == NULL) {
    //      SDL_LogError(0, "failed to load font: %s\n", SDL_GetError());
    //    }

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
                                        SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    //   audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &desiredAudioConfig,
    //   &config,
    //  0);
    if (audioDeviceID == 0) {
      SDL_LogError(0, "Couldn't open audio: %s\n", SDL_GetError());
      return false;
    } else {
      SDL_LogInfo(0,
                  "Audio status for "
                  "device# %d: %s\n",
                  audioDeviceID, SDL_GetAudioDeviceName(audioDeviceID, 0));
    }
    SDL_PauseAudioDevice(audioDeviceID,
                         0); // start playback

    SDL_SetRenderDrawColor(renderer, 0, 0, 0,
                           0);   // setting draw color
    SDL_RenderClear(renderer);   // Clear the newly
                                 // created window
    SDL_RenderPresent(renderer); // Reflects the
                                 // changes done in the
    //  window.
    if (!loadMedia()) {
      printf("Media could not be "
             "loaded...\n");
    }

    if (std::atomic<float>{}.is_lock_free()) {
      SDL_LogInfo(0, "atomic is lock free!");
    } else {
      SDL_LogError(0, "no hardware atomic... "
                      "using mutex fallback!");
    }

    // physics.gravity = vec2f_t{.x = 4, .y = 9.8};
    lastFrameTime = SDL_GetTicks();
    // init was successful

    std::stringstream ss;
    ss << "audio conf: \n";
    ss << "format: " << config.format << "\n";
    ss << "channels: " << config.channels << "\n";
    ss << "freq: " << config.freq << "\n";
    ss << "padding: " << config.padding << "\n";
    ss << "silence: " << config.silence << "\n";
    ss << "size: " << config.size << "\n";
    SDL_LogInfo(0, "%s", ss.str().c_str());
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
    //    TTF_CloseFont(font);
    //    font = NULL;
    //    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
  }

  void audioCallback(Uint8 *stream, int len) {

    /* 2 channels, 4 bytes/sample = 8
     * bytes/frame */
    float *sampleStream = (float *)stream;
    size_t numRequestedSamples = len / (config.channels * 4);
    for (size_t i = 0; i < numRequestedSamples; i++) {

      sampleStream[i * config.channels] =
          sampleStream[i * config.channels + 1] = synth.next();
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
    bool success = true;

    auto path = "images/p04_shape1.bmp";
    SDL_Surface *surface = IMG_Load(path);
    gCursor = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (gCursor == NULL) {
      SDL_LogError(0,
                   "Unable to load image %s! "
                   "SDL Error: %s\n",
                   path, SDL_GetError());
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
        break;
      }
      case SDL_MOUSEBUTTONUP: {

        // auto mDown = vec2f_t{.x = float(mouseDownPositionX),
        //                      .y = float(mouseDownPositionY)};
        // auto mUp =
        //     vec2f_t{.x = float(event.motion.x), .y = float(event.motion.y)};

        break;
      }
      case SDL_MULTIGESTURE: {
        break;
      }
      case SDL_JOYAXISMOTION: {

        break;
      }
      case SDL_SENSORUPDATE: {

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
  }

  void draw(SDL_Event &event) {
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;
    SDL_RenderClear(renderer);
    // drawing code here

    for (auto object : gameObjects) {
    }

    //    auto textSurface =
    //    TTF_RenderUTF8_LCD(font,
    //    ss.str().c_str(), textColor,
    //                                          textBackgroundColor);
    //    auto textTexture =
    //    SDL_CreateTextureFromSurface(renderer,
    //    textSurface); SDL_Rect
    //    textSrcRect =
    //        SDL_Rect{.x = 0, .y = 0, .w
    //        = textSurface->w, .h =
    //        textSurface->h};
    //    SDL_Rect textDestRect =
    //    SDL_Rect{.x = 25, .y = 25, .w =
    //    500, .h = 200};
    //    SDL_RenderCopy(renderer,
    //    textTexture, &textSrcRect,
    //    &textDestRect);
    //    SDL_FreeSurface(textSurface);
    //    SDL_DestroyTexture(textTexture);
    SDL_RenderPresent(renderer);
  }

private:
  const int FRAME_RATE_TARGET = 60; // 5; // 120
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
  TTF_Font *font = NULL;
  std::vector<GameObject *> gameObjects;
  GameObject *wall1, *wall2, *wall3, *wall4;
  SDL_Color textColor = {20, 20, 20};
  SDL_Color textBackgroundColor = {0, 0, 0};
  double lastAccZ = 9.8;
  int mousePositionX = 0;
  int mousePositionY = 0;
  int mouseDownPositionX = 0;
  int mouseDownPositionY = 0;
  int joystickXPosition = 0;
  int joystickYPosition = 0;
  float xDir = 0, yDir = 0;
  SDL_AudioSpec config;
  Synthesizer<float> synth;
  int lastFrameTime = 0;
  int radius = 50;
  bool renderIsOn = true;
  int audioDeviceID = -1;
  double previousAccelerationZ = 9.8;
  Physics physics;
  double frameDeltaTimeSeconds;
  double timeOfLastChordChange;
  int chordSetIndex = 0;
};

int main(int argc, char *argv[]) {
  SDL_Log("begin!");
  Framework game(2220, 940);
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
