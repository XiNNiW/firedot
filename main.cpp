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
#include "include/ui.h"
#include "include/vector_math.h"
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
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <map>

#include <vector>

#ifndef SDL_AUDIODRIVER
#define SDL_AUDIODRIVER "jack"
#endif // !SDL_AUDIODRIVER

class Framework {
public:
  static inline void forwardAudioCallback(void *userdata, Uint8 *stream,
                                          int len) {
    static_cast<Framework *>(userdata)->audioCallback(stream, len);
  }
  Framework(int width_, int height_)
      : height(height_), width(width_),
        synth(Synthesizer<float>(SubtractiveSynthesizer<float>())) {
    ;
  }

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

    auto font = TTF_OpenFont("fonts/Roboto-Medium.ttf", 16);
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
    SDL_LogInfo(0, "%s", ss.str().c_str());

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

    auto pageMargin = 50;
    auto radiobuttonMargin = 10;
    auto synthSelectWidth = width / 3.25;
    auto synthSelectHeight= width / 8.0;
    synthTypes = {SynthesizerType::SUBTRACTIVE, SynthesizerType::PHYSICAL_MODEL,
                  SynthesizerType::FREQUENCY_MODULATION};
    std::vector<std::string> synthTypeLabels = {"sub", "phys", "fm"};
    const size_t initialSynthTypeSelection = 0;
    for (size_t i = 0; i < synthTypes.size(); ++i) {
      synthSelectRadioGroup.push_back(Button{
          .labelText = synthTypeLabels[i],
          .shape = AxisAlignedBoundingBox{
              .position =
                  vec2f_t{.x = static_cast<float>(
                              pageMargin + synthSelectWidth / 2.0 +
                              i * (synthSelectWidth + radiobuttonMargin)),
                          .y = static_cast<float>(pageMargin +
                                                  synthSelectHeight / 2.0)},
              .halfSize =
                  vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
                          .y = static_cast<float>(synthSelectHeight / 2)}}});
    }
    synthSelectRadioGroup[initialSynthTypeSelection].state = UIState::ACTIVE;
    for (auto &button : synthSelectRadioGroup) {
      buttons.push_back(&button);
    }

    auto buttonMargin = 5;
    auto topBarHeight = synthSelectHeight + (1.5 * buttonMargin) + pageMargin;
    auto keySize = width / 4.5;
    auto keyboardStartPositionX = 100;
    int numberOfKeys = 24;

    for (size_t i = 0; i < numberOfKeys; ++i) {

      keyButtons.push_back(Button{
          .labelText = "",
          .shape = AxisAlignedBoundingBox{
              .position =
                  vec2f_t{.x = static_cast<float>(pageMargin + keySize / 2.0 +
                                                  (i % 4) *
                                                      (keySize + buttonMargin)),
                          .y = static_cast<float>(
                              topBarHeight + keySize / 2.0 +
                              floor(i / 4.0) * (keySize + buttonMargin))},
              .halfSize = vec2f_t{.x = static_cast<float>(keySize / 2),
                                  .y = static_cast<float>(keySize / 2)}}});

      noteListKeys.push_back(48 + i * 2);
    }

    for (auto &button : keyButtons) {
      buttons.push_back(&button);
    }

    synth.setSynthType(synthTypes[initialSynthTypeSelection]);
    synth.setGain(1);
    synth.setFilterCutoff(10000);
    synth.setFilterQuality(0.5);
    synth.setSoundSource(0.5);
    synth.setAttackTime(10.0);
    synth.setReleaseTime(1000.0);

    lastFrameTime = SDL_GetTicks();

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
    handleEvents(event);
    if (event.type == SDL_QUIT || (!renderIsOn))
      return;

    updatePhysics(event);
    evaluateGameRules(event);
  }

  void handleEvents(SDL_Event &event) {
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
        mousePosition.x = event.motion.x;
        mousePosition.y = event.motion.y;
        for (auto& button : synthSelectRadioGroup) {
          if ((button.state != UIState::ACTIVE) &&
              button.shape.contains(mousePosition)) {
            button.state = UIState::HOVER;
          } else if (button.state == UIState::HOVER) {
            button.state = UIState::INACTIVE;
          }
        }
        break;
      case SDL_MOUSEBUTTONDOWN: {
        mouseDownPosition.x = event.motion.x;
        mouseDownPosition.y = event.motion.y;

        auto selected =
            DoClickRadioGroup(synthSelectRadioGroup.data(),
                              synthSelectRadioGroup.size(), mousePosition);
        if (synthTypes[selected] != synth.type) {
          synth.setSynthType(synthTypes[selected]);
        };

     //   for (size_t i = 0; i < keyButtons.size(); ++i) {
     //     // evaluate clicks
     //     if (UpdateButton(&keyButtons[i], mousePosition, UIState::ACTIVE)) {
     //       // play sound
     //       synth.note(noteListKeys[i], 127);
     //     }
     //   }

        break;
      }
      case SDL_MOUSEBUTTONUP: {

       for (size_t i = 0; i < keyButtons.size(); ++i) {
         if ((keyButtons[i].state == UIState::ACTIVE)) {
           keyButtons[i].state = UIState::INACTIVE;
           synth.note(noteListKeys[i], 0);
           heldKeys.clear();
         } else if ((keyButtons[i].state == UIState::HOVER)){
keyButtons[i].state = UIState::INACTIVE;
         }
       }

        break;
      }
      case SDL_MULTIGESTURE: {



        break;
      }
      case SDL_FINGERMOTION: {
       auto fingerId =  event.tfinger.fingerId;
       auto position = vec2f_t { .x= event.tfinger.x*width, .y=event.tfinger.y*height};
       auto velocity = event.tfinger.pressure * 100 + 17;
        for (auto button : buttons) {
          if ((button->state != UIState::ACTIVE) && (button->state != UIState::HOVER)&&
              button->shape.contains(position)) {
            button->state = UIState::HOVER;
          }
        }

                               break;}
      case SDL_FINGERDOWN:{
       auto fingerId =  event.tfinger.fingerId;
       auto position = vec2f_t { .x= event.tfinger.x*width, .y=event.tfinger.y*height};
       auto velocity = event.tfinger.pressure * 100 + 17;
SDL_Log("finger down #%d: %f %f", fingerId, position.x, position.y);
       for (size_t i = 0; i < keyButtons.size(); ++i) {
          // evaluate clicks
          if (UpdateButton(&keyButtons[i], position, UIState::ACTIVE)) {
            // play sound
           auto note = noteListKeys[i];
           heldKeys[fingerId] = i;
          synth.note(note, velocity);
          }
        }

       break;
    }
      case SDL_FINGERUP:{

       auto fingerId =  event.tfinger.fingerId;
       auto position = vec2f_t { .x= event.tfinger.x*width, .y=event.tfinger.y*height};
SDL_Log("finger up #%d: %f %f", fingerId, position.x, position.y);
       if(auto buttonIdx = heldKeys[fingerId]){
         keyButtons[buttonIdx].state = UIState::INACTIVE;
            synth.note(noteListKeys[buttonIdx], 0);
            heldKeys.erase(fingerId);
       }

       if(heldKeys.size()==0){
        for (auto& button : keyButtons) {
          // evaluate clicks
           button.state = UIState::INACTIVE;
        }

       }

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

          synth.setFilterCutoff((event.sensor.data[0] / 9.8) );
          synth.setSoundSource((event.sensor.data[1] / 9.8));
          synth.setFilterQuality((event.sensor.data[2] / 9.8));

          break;
        }
        case SDL_SENSOR_GYRO: {
          ss << "sensor: " << sensorName << " " << event.sensor.data[0] << ", "
             << event.sensor.data[1] << ", " << event.sensor.data[2];
          // SDL_LogInfo(0, "%s", ss.str().c_str());
          break;
        }
        default:
          break;
        }
        break;
      }
      }
    }
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
    for (auto button : buttons) {
      DrawButtonRect(*button, renderer, style);
      DrawButtonLabel(*button, renderer, style);
    }
    SDL_RenderPresent(renderer);
  }

private:
  const int FRAME_RATE_TARGET =  120;
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
  std::vector<GameObject *> gameObjects;
  GameObject *wall1, *wall2, *wall3, *wall4;
  std::vector<Button> keyButtons;
  std::vector<float> noteListKeys;
  std::vector<Button> synthSelectRadioGroup;
  std::vector<SynthesizerType> synthTypes;

  std::vector<Button *> buttons;

  Style style;

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
  Synthesizer<float> synth;
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
