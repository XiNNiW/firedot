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
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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

enum IconType { MENU, SYNTH_SELECT };

static const size_t NUM_SYNTH_TYPES = 3;
static const SynthesizerType synthTypes[NUM_SYNTH_TYPES] = {
    SynthesizerType::SUBTRACTIVE, SynthesizerType::PHYSICAL_MODEL,
    SynthesizerType::FREQUENCY_MODULATION};

enum SensorType { TILT, SPIN };

struct UI {
  virtual void show(const float width, const float height) = 0;
  virtual void handleMouseMove(const vec2f_t &mousePosition) = 0;
  virtual void handleMouseDown(const vec2f_t &mousePosition) = 0;
  virtual void handleMouseUp(const vec2f_t &mousePosition) = 0;

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position,
                                const float pressure) = 0;
  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position,
                                const float pressure) = 0;
  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position,
                              const float pressure) = 0;

  virtual const std::vector<Button *> &getAllWidgets() = 0;
  virtual ~UI(){};
};

template <typename sample_t> struct SensorMapping {
  std::set<std::pair<SensorType,
                     typename ParameterChangeEvent<sample_t>::ParameterType>>
      mapping;

  inline void emitEvent(Synthesizer<sample_t> *synth, SensorType type,
                        float value) {
    for (auto &pair : mapping) {
      auto sensorType = pair.first;
      auto parameterEventType = pair.second;
      if (type == sensorType) {
        synth->pushParameterChangeEvent(parameterEventType, value);
      }
    }
  }

  inline void
  addMapping(SensorType sensorType,
             typename ParameterChangeEvent<sample_t>::ParameterType paramType) {
    mapping.insert(std::pair(sensorType, paramType));
  }

  inline void removeMapping(
      SensorType sensorType,
      typename ParameterChangeEvent<sample_t>::ParameterType paramType) {
    mapping.erase(std::pair(sensorType, paramType));
  }
};

struct KeyboardWidget {
  static constexpr size_t SYNTH_SELECTED_RADIO_GROUP_SIZE = 3;
  Button synthSelectRadioGroup[SYNTH_SELECTED_RADIO_GROUP_SIZE];
  std::vector<Button> keyButtons;
  Button menuButton;
  std::vector<Button *> allButtons;

  KeyboardWidget() {}

  template <typename sample_t>
  inline void buildLayout(const float width, const float height,
                          const std::vector<sample_t> &notes) {

    auto pageMargin = 50;
    auto radiobuttonMargin = 10;
    auto synthSelectWidth = width / 6;
    auto synthSelectHeight = width / 8.0;

    std::array<std::string, NUM_SYNTH_TYPES> synthTypeLabels = {"sub", "phys",
                                                                "fm"};
    const size_t initialSynthTypeSelection = 0;
    for (size_t i = 0; i < NUM_SYNTH_TYPES; ++i) {
      synthSelectRadioGroup[i] = Button{
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
                          .y = static_cast<float>(synthSelectHeight / 2)}}};
    }
    synthSelectRadioGroup[initialSynthTypeSelection].state = UIState::ACTIVE;
    for (auto &button : synthSelectRadioGroup) {
      allButtons.push_back(&button);
    }

    menuButton = Button{
        .labelText = "menu",
        .shape = AxisAlignedBoundingBox{
            .position = vec2f_t{.x = width - synthSelectWidth / 2 - pageMargin,
                                .y = static_cast<float>(pageMargin +
                                                        synthSelectHeight / 2)},
            .halfSize =
                vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
                        .y = static_cast<float>(synthSelectHeight / 2)}}};

    allButtons.push_back(&menuButton);

    auto buttonMargin = 5;
    auto topBarHeight = synthSelectHeight + (1.5 * buttonMargin) + pageMargin;
    auto keySize = width / 4.5;
    auto keyboardStartPositionX = 100;

    for (size_t i = 0; i < notes.size(); ++i) {

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
    }

    for (auto &button : keyButtons) {
      allButtons.push_back(&button);
    }
  }

  const std::vector<Button *> &getAllWidgets() { return allButtons; }
};

struct KeyboardController {

  std::map<SDL_FingerID, int> heldKeys;

  template <typename sample_t>
  inline void handleFingerMove(KeyboardWidget *keyboardWidget,
                               Synthesizer<sample_t> *synth,
                               const std::vector<sample_t> &notes,
                               const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (auto button : keyboardWidget->getAllWidgets()) {
      if ((button->state != UIState::ACTIVE) &&
          (button->state != UIState::HOVER) &&
          button->shape.contains(position)) {
        button->state = UIState::HOVER;
      }
    }
  }

  template <typename sample_t>
  inline void handleFingerDown(KeyboardWidget *keyboardWidget,
                               Synthesizer<sample_t> *synth,
                               const std::vector<sample_t> &notes,
                               const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < keyboardWidget->keyButtons.size(); ++i) {
      // evaluate clicks
      if (UpdateButton(&keyboardWidget->keyButtons[i], position,
                       UIState::ACTIVE)) {
        // play sound
        auto note = notes[i];
        heldKeys[fingerId] = i;
        synth->note(note, pressure * 127);
      }
    }
  }

  template <typename sample_t>
  inline void handleFingerUp(KeyboardWidget *keyboardWidget,
                             Synthesizer<sample_t> *synth,
                             const std::vector<sample_t> &notes,
                             const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {

    if (auto buttonIdx = heldKeys[fingerId]) {
      keyboardWidget->keyButtons[buttonIdx].state = UIState::INACTIVE;
      synth->note(notes[buttonIdx], 0);
      heldKeys.erase(fingerId);
    }

    if (heldKeys.size() == 0) {
      for (auto &button : keyboardWidget->keyButtons) {
        // evaluate clicks
        button.state = UIState::INACTIVE;
      }
    }
  }

  template <typename sample_t>
  inline void handleMouseMove(KeyboardWidget *keyboardWidget,
                              Synthesizer<sample_t> *synth,
                              const std::vector<sample_t> &notes,
                              const vec2f_t &mousePosition) {
    for (auto &button : keyboardWidget->synthSelectRadioGroup) {
      if ((button.state != UIState::ACTIVE) &&
          button.shape.contains(mousePosition)) {
        button.state = UIState::HOVER;
      } else if (button.state == UIState::HOVER) {
        button.state = UIState::INACTIVE;
      }
    }
  }

  template <typename sample_t>
  inline void handleMouseDown(KeyboardWidget *keyboardWidget,
                              Synthesizer<sample_t> *synth,
                              const std::vector<sample_t> &notes,
                              const vec2f_t &mousePosition) {
    auto selected = DoClickRadioGroup(
        keyboardWidget->synthSelectRadioGroup,
        KeyboardWidget::SYNTH_SELECTED_RADIO_GROUP_SIZE, mousePosition);
    if (synthTypes[selected] != synth->type) {
      synth->setSynthType(synthTypes[selected]);
    };

    if (UpdateButton(&keyboardWidget->menuButton, mousePosition, ACTIVE)) {
    };

    //  for (size_t i = 0; i < keyboardWidget->keyButtons.size(); ++i) {
    //    // evaluate clicks
    //    if (UpdateButton(&keyboardWidget->keyButtons[i], mousePosition,
    //                     UIState::ACTIVE)) {
    //      // play sound
    //      synth->note(notes[i], 127);
    //    }
    //  }
  }

  template <typename sample_t>
  inline void handleMouseUp(KeyboardWidget *keyboardWidget,
                            Synthesizer<sample_t> *synth,
                            const std::vector<sample_t> &notes,
                            const vec2f_t &mousePosition) {

    for (size_t i = 0; i < keyboardWidget->keyButtons.size(); ++i) {
      if (keyboardWidget->keyButtons[i].state == UIState::ACTIVE) {
        keyboardWidget->keyButtons[i].state = UIState::INACTIVE;
        synth->note(notes[i], 0);
        heldKeys.clear();
      } else if (keyboardWidget->keyButtons[i].state == UIState::HOVER) {
        keyboardWidget->keyButtons[i].state = UIState::INACTIVE;
      }
    }
  }
};

struct KeyboardUI : UI {
  KeyboardWidget widget;
  KeyboardController controller;
  Synthesizer<float> *synth = NULL;
  std::vector<float> notes = {36, 40, 44, 48, 43, 47, 51, 55, 50, 54, 58, 62,
                              57, 61, 65, 69, 64, 68, 72, 76, 71, 75, 79, 83};

  KeyboardUI(Synthesizer<float> *_synth) : synth(_synth) {}
  ~KeyboardUI() {}

  void show(const float width, const float height) {
    widget.buildLayout(width, height, notes);
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    controller.handleMouseMove(&widget, synth, notes, mousePosition);
  }
  inline void handleMouseDown(const vec2f_t &mousePosition) {
    controller.handleMouseDown(&widget, synth, notes, mousePosition);
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {
    controller.handleMouseUp(&widget, synth, notes, mousePosition);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    controller.handleFingerMove(&widget, synth, notes, fingerId, position,
                                pressure);
  }
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    controller.handleFingerDown(&widget, synth, notes, fingerId, position,
                                pressure);
  }
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    controller.handleFingerUp(&widget, synth, notes, fingerId, position,
                              pressure);
  }
  inline const std::vector<Button *> &getAllWidgets() {
    return widget.getAllWidgets();
  }
};
struct OptionMenu {
  AxisAlignedBoundingBox shape =
      AxisAlignedBoundingBox{.position = {0, 0}, .halfSize = {100, 100}};
  UIState state = INACTIVE;
};

struct MappingWidget {
  OptionMenu tiltMappingMenu;
  std::vector<Button *> allWidgets;

  MappingWidget() {}

  void buildLayout(const float width, const float height) {
    tiltMappingMenu.shape =
        AxisAlignedBoundingBox{.position = {.x = width / 2, .y = height / 2}};
  }

  inline const std::vector<Button *> &getAllWidgets() { return allWidgets; }
};

struct MappingController {
  inline void handleMouseMove(MappingWidget *widget,
                              SensorMapping<float> *mapping,
                              const vec2f_t &mousePosition) {}
  inline void handleMouseDown(MappingWidget *widget,
                              SensorMapping<float> *mapping,
                              const vec2f_t &mousePosition) {}
  inline void handleMouseUp(MappingWidget *widget,
                            SensorMapping<float> *mapping,
                            const vec2f_t &mousePosition) {}

  inline void handleFingerMove(MappingWidget *widget,
                               SensorMapping<float> *mapping,
                               const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerDown(MappingWidget *widget,
                               SensorMapping<float> *mapping,
                               const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerUp(MappingWidget *widget,
                             SensorMapping<float> *mapping,
                             const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}
};

struct MappingUI : UI {
  MappingWidget widget;
  MappingController controller;
  SensorMapping<float> *mapping = NULL;

  MappingUI(SensorMapping<float> *_mapping) : mapping(_mapping) {}
  ~MappingUI() {}
  void show(const float width, const float height) {
    widget.buildLayout(width, height);
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    controller.handleMouseMove(&widget, mapping, mousePosition);
  }
  inline void handleMouseDown(const vec2f_t &mousePosition) {
    controller.handleMouseDown(&widget, mapping, mousePosition);
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {
    controller.handleMouseUp(&widget, mapping, mousePosition);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    controller.handleFingerMove(&widget, mapping, fingerId, position, pressure);
  }
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    controller.handleFingerDown(&widget, mapping, fingerId, position, pressure);
  }
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    controller.handleFingerUp(&widget, mapping, fingerId, position, pressure);
  }
  inline const std::vector<Button *> &getAllWidgets() {
    return widget.getAllWidgets();
  }
};

struct UIRenderer {

  static inline void drawUI(SDL_Renderer *renderer, UI *ui,
                            const Style &style) {

    auto allWidgets = ui->getAllWidgets();
    for (auto button : allWidgets) {
      DrawButtonRect(*button, renderer, style);
      DrawButtonLabel(*button, renderer, style);
    }
  }
};

inline const std::optional<SDL_Texture *>
LoadIconTexture(SDL_Renderer *renderer, std::string path) {
  bool success = true;

  SDL_Surface *surface = IMG_Load(path.c_str());
  auto texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  if (texture == NULL) {
    SDL_LogError(0,
                 "Unable to load image %s! "
                 "SDL Error: %s\n",
                 path.c_str(), SDL_GetError());
    return std::nullopt;
  } else {

    return texture;
  }
}

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

    synth.setSynthType(SUBTRACTIVE);
    synth.setGain(1);
    synth.setFilterCutoff(10000);
    synth.setFilterQuality(0.5);
    synth.setSoundSource(0.5);
    synth.setAttackTime(10.0);
    synth.setReleaseTime(1000.0);

    ui = new KeyboardUI(&synth);
    ui->show(width, height);

    sensorMapping.addMapping(
        TILT, ParameterChangeEvent<float>::ParameterType::SOUND_SOURCE);
    sensorMapping.addMapping(
        SPIN, ParameterChangeEvent<float>::ParameterType::FILTER_CUTOFF);

    lastFrameTime = SDL_GetTicks();

    return true;
  }

  ~Framework() {
    delete ui;
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

    auto maybeCursor = LoadIconTexture(renderer, "images/p04_shape1.bmp");
    if (maybeCursor.has_value()) {
      gCursor = maybeCursor.value();
    } else {
      return false;
    }

    auto maybeMenuIcon =
        LoadIconTexture(renderer, "images/menu-burger-svgrepo-com.svg");
    if (maybeMenuIcon.has_value()) {
      menuIcon = maybeMenuIcon.value();
    } else {
      return false;
    }

    auto maybeSynthSelectIcon =
        LoadIconTexture(renderer, "images/colour-tuneing-svgrepo-com.svg");
    if (maybeSynthSelectIcon.has_value()) {
      synthSelectIcon = maybeSynthSelectIcon.value();
    } else {
      return false;
    }

    return true;
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
        ui->handleMouseMove(mousePosition);
        break;
      case SDL_MOUSEBUTTONDOWN: {
        mouseDownPosition.x = event.motion.x;
        mouseDownPosition.y = event.motion.y;

        ui->handleMouseDown(mouseDownPosition);

        break;
      }
      case SDL_MOUSEBUTTONUP: {

        ui->handleMouseUp(mousePosition);

        break;
      }
      case SDL_MULTIGESTURE: {

        break;
      }
      case SDL_FINGERMOTION: {
        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};
        ui->handleFingerMove(fingerId, position, event.tfinger.pressure);
        break;
      }
      case SDL_FINGERDOWN: {
        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};

        ui->handleFingerDown(fingerId, position, event.tfinger.pressure);

        break;
      }
      case SDL_FINGERUP: {

        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};
        SDL_Log("finger up #%ld: %f %f", fingerId, position.x, position.y);
        ui->handleFingerUp(fingerId, position, event.tfinger.pressure);

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

          auto accVector =
              vec2f_t{.x = event.sensor.data[0] * -1, .y = event.sensor.data[1]}
                  .scale(1.0 / 9.8);
          ;

          // synth.setSoundSource(accVector.length());
          sensorMapping.emitEvent(&synth, SensorType::TILT, accVector.length());

          break;
        }
        case SDL_SENSOR_GYRO: {
          ss << "sensor: " << sensorName << " " << event.sensor.data[0] << ", "
             << event.sensor.data[1] << ", " << event.sensor.data[2];
          // SDL_LogInfo(0, "%s", ss.str().c_str());

          sensorMapping.emitEvent(&synth, SensorType::SPIN,
                                  event.sensor.data[2]);
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

    UIRenderer::drawUI(renderer, ui, style);

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
  std::vector<GameObject *> gameObjects;
  GameObject *wall1, *wall2, *wall3, *wall4;
  std::vector<float> noteListKeys;
  std::vector<Button> synthSelectRadioGroup;
  std::vector<SynthesizerType> synthTypes;

  Style style;
  SDL_Texture *menuIcon = NULL;
  SDL_Texture *synthSelectIcon = NULL;

  UI *ui;

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
  SensorMapping<float> sensorMapping;
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
