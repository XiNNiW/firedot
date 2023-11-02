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

enum SensorType { TILT, SPIN };
static const size_t NUM_SENSOR_TYPES = 2;
static_assert(SPIN == NUM_SENSOR_TYPES - 1, "enum and table size must agree");
static const SensorType SensorTypes[NUM_SENSOR_TYPES] = {TILT, SPIN};
static const char *SensorTypesDisplayNames[NUM_SENSOR_TYPES] = {"tilt", "spin"};

template <typename sample_t> struct SensorMapping {
  // std::set<std::pair<SensorType, ParameterType>> mapping;
  std::map<SensorType, ParameterType> mapping;

  inline void emitEvent(Synthesizer<sample_t> *synth, SensorType type,
                        sample_t value) {
    for (auto &pair : mapping) {
      auto sensorType = pair.first;
      auto parameterEventType = pair.second;
      if (type == sensorType) {
        synth->pushParameterChangeEvent(parameterEventType, value);
      }
    }
  }

  inline void addMapping(SensorType sensorType, ParameterType paramType) {
    // mapping.insert(std::pair(sensorType, paramType));
    mapping[sensorType] = paramType;
  }

  inline void removeMapping(SensorType sensorType, ParameterType paramType) {
    // mapping.erase(std::pair(sensorType, paramType));
    mapping.erase(sensorType);
  }
};
struct Navigation {
  enum Page { KEYBOARD, MAPPING } page = KEYBOARD;
};
struct KeyboardUI {

  static constexpr size_t SYNTH_SELECTED_RADIO_GROUP_SIZE = 3;
  static constexpr size_t NUM_KEY_BUTTONS = 24;
  float notes[NUM_KEY_BUTTONS] = {36, 40, 44, 48, 43, 47, 51, 55,
                                  50, 54, 58, 62, 57, 61, 65, 69,
                                  64, 68, 72, 76, 71, 75, 79, 83};
  Navigation *navigation = NULL;
  Synthesizer<float> *synth = NULL;

  std::map<SDL_FingerID, int> heldKeys;

  Button menuButton;
  Button synthSelectRadioGroup[SYNTH_SELECTED_RADIO_GROUP_SIZE];
  Button keyButtons[NUM_KEY_BUTTONS];

  float synthSelectWidth = 50;
  float synthSelectHeight = 50;
  float buttonMargin = 5;
  void buildLayout(const float width, const float height) {
    auto pageMargin = 50;
    auto radiobuttonMargin = 10;
    synthSelectWidth = width / 6;
    synthSelectHeight = width / 8.0;

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
    menuButton = Button{
        .labelText = "menu",
        .shape = AxisAlignedBoundingBox{
            .position = vec2f_t{.x = width - synthSelectWidth / 2 - pageMargin,
                                .y = static_cast<float>(pageMargin +
                                                        synthSelectHeight / 2)},
            .halfSize =
                vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
                        .y = static_cast<float>(synthSelectHeight / 2)}}};

    auto topBarHeight = synthSelectHeight + (1.5 * buttonMargin) + pageMargin;
    auto keySize = width / 4.5;
    auto keyboardStartPositionX = 100;

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {

      keyButtons[i] = Button{
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
                                  .y = static_cast<float>(keySize / 2)}}};
    }
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (auto &button : synthSelectRadioGroup) {
      DoButtonHover(&button, position);
    }
    for (auto &button : keyButtons) {
      DoButtonHover(&button, position);
    }
    DoButtonHover(&menuButton, position);
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      // evaluate clicks
      if (DoButtonClick(&keyButtons[i], position, UIState::ACTIVE)) {
        // play sound
        auto note = notes[i];
        heldKeys[fingerId] = i;
        synth->note(note, pressure * 127);
      }
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {

    if (auto buttonIdx = heldKeys[fingerId]) {
      keyButtons[buttonIdx].state = UIState::INACTIVE;
      synth->note(notes[buttonIdx], 0);
      heldKeys.erase(fingerId);
    }

    if (heldKeys.size() == 0) {
      for (auto &button : keyButtons) {
        button.state = UIState::INACTIVE;
      }
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    for (auto &button : synthSelectRadioGroup) {
      if ((button.state != UIState::ACTIVE) &&
          button.shape.contains(mousePosition)) {
        button.state = UIState::HOVER;
      } else if (button.state == UIState::HOVER) {
        button.state = UIState::INACTIVE;
      }
    }
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    auto selected = DoClickRadioGroup(
        synthSelectRadioGroup, SYNTH_SELECTED_RADIO_GROUP_SIZE, mousePosition);
    if (SynthTypes[selected] != synth->type) {
      synth->setSynthType(SynthTypes[selected]);
    };

    if (DoButtonClick(&menuButton, mousePosition, ACTIVE)) {
      navigation->page = Navigation::MAPPING;
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

  inline void handleMouseUp(const vec2f_t &mousePosition) {

    menuButton.state = INACTIVE;

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (keyButtons[i].state == UIState::ACTIVE) {
        keyButtons[i].state = UIState::INACTIVE;
        synth->note(notes[i], 0);
        heldKeys.clear();
      } else if (keyButtons[i].state == UIState::HOVER) {
        keyButtons[i].state = UIState::INACTIVE;
      }
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    DrawButton(menuButton, renderer, style);

    auto radiogroupBackgroundRect = SDL_Rect{
        .x = static_cast<int>(synthSelectRadioGroup[0].shape.position.x -
                              synthSelectRadioGroup[0].shape.halfSize.x - 5),
        .y = static_cast<int>(synthSelectRadioGroup[0].shape.position.y -
                              synthSelectRadioGroup[0].shape.halfSize.y - 5),
        .w = static_cast<int>(
            (synthSelectWidth + buttonMargin) * NUM_SYNTH_TYPES + 15),
        .h = static_cast<int>(synthSelectHeight + 10)};
    SDL_SetRenderDrawColor(renderer, style.hoverColor.r, style.hoverColor.g,
                           style.hoverColor.b, style.hoverColor.a);
    SDL_RenderFillRect(renderer, &radiogroupBackgroundRect);
    for (size_t i = 0; i < NUM_SYNTH_TYPES; i++) {
      DrawButton(synthSelectRadioGroup[i], renderer, style);
    }

    for (size_t i = 0; i < NUM_KEY_BUTTONS; i++) {
      DrawButton(keyButtons[i], renderer, style);
    }
  }
};

struct MultiSelectMenu {
  enum Action { NOTHING, MENU_OPEN_CLICKED, MENU_SELECTION_CHANGED };
  UIState state = INACTIVE;
  float width = 500, height = 500, topMargin = 50, sideMargin = 15,
        titleBarHeight = 100;
  size_t selected = 0;
  Button closeButton;
  Button menuButton;
  std::string menuLabel;
  std::vector<Button> options;

  inline void buildLayout(const float _width, const float _height) {
    width = _width;
    height = _height;
    rebuildLayout();
  }
  inline void rebuildLayout() {
    auto numOptions = options.size();
    auto menuHeight = height - 2 * topMargin - titleBarHeight;
    auto menuWidth = width - 2 * sideMargin;
    auto menuCenter = vec2f_t{.x = static_cast<float>(width / 2.0),
                              .y = static_cast<float>(height / 2.0)};

    auto closeButtonSize = vec2f_t{.x = static_cast<float>(width / 8.0),
                                   .y = static_cast<float>(height / 12.0)};
    closeButton =
        Button{.labelText = "<- close",
               .shape = AxisAlignedBoundingBox{
                   .position = {.x = static_cast<float>(
                                    sideMargin + closeButtonSize.x / 2.0),
                                .y = static_cast<float>(
                                    topMargin + closeButtonSize.y / 2.0)},
                   .halfSize = closeButtonSize.scale(0.5)}};

    for (size_t i = 0; i < numOptions; ++i) {
      auto buttonSize = vec2f_t{.x = menuWidth, .y = menuHeight / numOptions};
      auto buttonPosition =
          vec2f_t{.x = static_cast<float>(width / 2.0),
                  .y = static_cast<float>((i * buttonSize.y) + titleBarHeight +
                                          topMargin + buttonSize.y / 2.0)};
      options[i].shape = AxisAlignedBoundingBox{
          .position = buttonPosition, .halfSize = buttonSize.scale(0.5)};
      if (i == selected) {
        options[i].state = ACTIVE;
      }
    }
  }
  inline void setOptions(const std::vector<Button> &optionButtons) {
    options = optionButtons;
    rebuildLayout();
  }
};

inline static MultiSelectMenu::Action
DoMultiSelectClick(MultiSelectMenu *menu, const vec2f_t &position) {
  if (menu->state != ACTIVE && menu->menuButton.shape.contains(position)) {
    menu->state = ACTIVE;
    return MultiSelectMenu::MENU_OPEN_CLICKED;
  } else if (menu->state == ACTIVE) {

    if (DoButtonClick(&menu->closeButton, position)) {
      menu->state = INACTIVE;
      return MultiSelectMenu::NOTHING;
    }

    size_t selected = 0;
    auto previousSelection = menu->selected;
    for (auto &button : menu->options) {

      if (DoButtonClick(&button, position)) {
        menu->selected = selected;
        menu->state = INACTIVE;
        break;
      }
      ++selected;
    }
    if (menu->selected != previousSelection) {
      for (size_t i = 0; i < menu->options.size(); ++i) {
        if (i == menu->selected) {
          menu->options[i].state = ACTIVE;
        } else {
          menu->options[i].state = INACTIVE;
        }
      }
      return MultiSelectMenu::MENU_SELECTION_CHANGED;
    }
  }
  return MultiSelectMenu::NOTHING;
}

inline static void DrawMultiSelectMenu(const MultiSelectMenu &menu,
                                       SDL_Renderer *renderer,
                                       const Style &style) {
  if (menu.state == ACTIVE) {

    auto screenRect = SDL_Rect{.x = 0,
                               .y = 0,
                               .w = static_cast<int>(menu.width),
                               .h = static_cast<int>(menu.height)};

    SDL_SetRenderDrawColor(renderer, style.inactiveColor.r,
                           style.inactiveColor.g, style.inactiveColor.b, 0xb0);
    SDL_RenderFillRect(renderer, &screenRect);
    DrawButton(menu.closeButton, renderer, style);
    for (auto &button : menu.options) {
      DrawButton(button, renderer, style);
    }
  } else {
    DrawButton(menu.menuButton, renderer, style);
  }
}

struct MappingUI {
  Button backButton;
  MultiSelectMenu sensorMenus[NUM_SENSOR_TYPES];

  SensorMapping<float> *sensorMapping = NULL;
  Navigation *navigation = NULL;

  float titleBarHeight = 100;
  float sideMargin = 15;
  float topMargin = 50;

  void buildLayout(const float width, const float height) {
    auto backButtonSize = vec2f_t{
        .x = static_cast<float>(width / 8.0),
        .y = static_cast<float>(width / 16.0),
    };
    backButton = Button{
        .labelText = "<- back",
        .shape = AxisAlignedBoundingBox{
            .position = {.x = static_cast<float>(backButtonSize.x / 2.0),
                         .y = static_cast<float>(backButtonSize.y / 2.0)},
            .halfSize = backButtonSize.scale(0.5)}};

    std::vector<Button> options;
    for (size_t i = 0; i < NUM_PARAMETER_TYPES; ++i) {
      options.push_back(Button{.labelText = ParameterTypeDisplayNames[i]});
    }

    auto menuButtonSize =
        vec2f_t{.x = width - 2 * sideMargin,
                .y = (height - 2 * topMargin - titleBarHeight) /
                     float(NUM_SENSOR_TYPES)};
    auto menuButtonHalfSize = menuButtonSize.scale(0.5);
    for (auto &sensorType : SensorTypes) {

      sensorMenus[sensorType] = MultiSelectMenu{
          .selected = sensorMapping->mapping[sensorType],
          .menuButton =
              Button{.labelText = SensorTypesDisplayNames[sensorType],
                     .shape =
                         AxisAlignedBoundingBox{
                             .position = {.x = static_cast<float>(width / 2.0),
                                          .y = sensorType * menuButtonSize.y +
                                               menuButtonHalfSize.y +
                                               topMargin + titleBarHeight},
                             .halfSize = menuButtonHalfSize.scale(0.5)}},
          .options = options,
      };
      sensorMenus[sensorType].buildLayout(width, height);
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {

    DoButtonHover(&backButton, mousePosition);
  }
  inline void handleMouseDown(const vec2f_t &mousePosition) {
    bool anyMenusActive = false;

    for (auto &sensorType : SensorTypes) {
      auto &menu = sensorMenus[sensorType];
      if (menu.state == ACTIVE) {
        anyMenusActive = true;
      }

      auto previousSelection = menu.selected;
      switch (DoMultiSelectClick(&menu, mousePosition)) {
      case MultiSelectMenu::MENU_OPEN_CLICKED:
        break;
      case MultiSelectMenu::MENU_SELECTION_CHANGED:
        sensorMapping->removeMapping(TILT, ParameterTypes[previousSelection]);
        sensorMapping->addMapping(TILT, ParameterTypes[menu.selected]);
        break;
      case MultiSelectMenu::NOTHING:
        break;
      }
    }

    if (!anyMenusActive && DoButtonClick(&backButton, mousePosition)) {
      navigation->page = Navigation::KEYBOARD;
    }
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {
    backButton.state = INACTIVE;
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    bool anyMenusActive = false;
    int activeSensorType = 0;

    for (auto &sensorType : SensorTypes) {
      if (sensorMenus[sensorType].state == ACTIVE) {
        anyMenusActive = true;
        activeSensorType = sensorType;
      }
    }

    if (anyMenusActive) {
      DrawMultiSelectMenu(sensorMenus[activeSensorType], renderer, style);
    } else {
      DrawButton(backButton, renderer, style);
      for (auto &menu : sensorMenus) {
        DrawMultiSelectMenu(menu, renderer, style);
      }
    }
  }
};

static inline const std::optional<SDL_Texture *>
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

    sensorMapping.addMapping(TILT, ParameterType::SOUND_SOURCE);
    sensorMapping.addMapping(SPIN, ParameterType::FILTER_CUTOFF);

    mappingUI.sensorMapping = &sensorMapping;
    mappingUI.navigation = &navigation;
    mappingUI.buildLayout(width, height);

    keyboardUI.synth = &synth;
    keyboardUI.navigation = &navigation;
    keyboardUI.buildLayout(width, height);

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
        switch (navigation.page) {
        case Navigation::KEYBOARD:
          keyboardUI.handleMouseMove(mousePosition);
          break;
        case Navigation::MAPPING:
          mappingUI.handleMouseMove(mousePosition);
          break;
        }

        break;
      case SDL_MOUSEBUTTONDOWN: {
        mouseDownPosition.x = event.motion.x;
        mouseDownPosition.y = event.motion.y;

        switch (navigation.page) {
        case Navigation::KEYBOARD:
          keyboardUI.handleMouseDown(mouseDownPosition);
          break;
        case Navigation::MAPPING:
          mappingUI.handleMouseDown(mouseDownPosition);
          break;
        }

        break;
      }
      case SDL_MOUSEBUTTONUP: {

        keyboardUI.handleMouseUp(mousePosition);
        switch (navigation.page) {
        case Navigation::KEYBOARD:
          keyboardUI.handleMouseUp(mouseDownPosition);
          break;
        case Navigation::MAPPING:
          mappingUI.handleMouseUp(mouseDownPosition);
          break;
        }

        break;
      }
      case SDL_MULTIGESTURE: {

        break;
      }
      case SDL_FINGERMOTION: {
        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};

        switch (navigation.page) {
        case Navigation::KEYBOARD:
          keyboardUI.handleFingerMove(fingerId, position,
                                      event.tfinger.pressure);
          break;
        case Navigation::MAPPING:
          mappingUI.handleFingerMove(fingerId, position,
                                     event.tfinger.pressure);
          break;
        }
        break;
      }
      case SDL_FINGERDOWN: {
        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};

        switch (navigation.page) {
        case Navigation::KEYBOARD:
          keyboardUI.handleFingerDown(fingerId, position,
                                      event.tfinger.pressure);

          break;
        case Navigation::MAPPING:
          mappingUI.handleFingerDown(fingerId, position,
                                     event.tfinger.pressure);
          break;
        }

        break;
      }
      case SDL_FINGERUP: {

        auto fingerId = event.tfinger.fingerId;
        auto position = vec2f_t{.x = event.tfinger.x * width,
                                .y = event.tfinger.y * height};
        SDL_Log("finger up #%ld: %f %f", fingerId, position.x, position.y);

        switch (navigation.page) {
        case Navigation::KEYBOARD:
          keyboardUI.handleFingerUp(fingerId, position, event.tfinger.pressure);
          break;
        case Navigation::MAPPING:
          mappingUI.handleFingerUp(fingerId, position, event.tfinger.pressure);
          break;
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

    switch (navigation.page) {

    case Navigation::KEYBOARD:
      keyboardUI.draw(renderer, style);
      break;
    case Navigation::MAPPING:
      mappingUI.draw(renderer, style);
      break;
    }

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

  Navigation navigation;
  KeyboardUI keyboardUI;
  MappingUI mappingUI;

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
