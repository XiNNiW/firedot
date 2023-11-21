#pragma once

#include "SDL_render.h"
#include "collider.h"
#include "sensor.h"
#include "synthesis.h"
#include "widget.h"
#include <cstddef>
#include <map>
#include <sstream>
enum IconType { MENU, SYNTH_SELECT };
struct Navigation {
  enum Page { KEYBOARD, MAPPING, SOUND_EDIT } page = KEYBOARD;
};
static const int NUM_NAVIGATION_PAGES = 3;
static_assert((NUM_NAVIGATION_PAGES - 1) == Navigation::SOUND_EDIT,
              "Navigation enum size does not match NavigationPages");
static const Navigation::Page NavigationPages[NUM_NAVIGATION_PAGES] = {
    Navigation::KEYBOARD, Navigation::MAPPING, Navigation::SOUND_EDIT};

static const char *NavigationPageDisplayNames[NUM_NAVIGATION_PAGES] = {
    "keyboard",
    "sensor mapping",
    "sound edit",
};

struct NavigationUI {
  RadioGroup pages;
  Navigation *navigation = NULL;
  float topMargin = 5;
  float pageMargin = 25;
  float bottomMargin = 15;
  float seperatorHeight = 2;
  float buttonHeight = 200;
  AxisAlignedBoundingBox shape;
  static inline const NavigationUI MakeNavigationUI(Navigation *navigation) {

    const size_t initialSynthTypeSelection = navigation->page;
    std::vector<std::string> labels;
    for (auto &page : NavigationPages) {
      labels.push_back(NavigationPageDisplayNames[page]);
    }

    return NavigationUI{
        .pages = RadioGroup::MakeRadioGroup(labels, initialSynthTypeSelection),
        .navigation = navigation};
    ;
  }
  inline void buildLayout(const float width, const float height) {

    buttonHeight = height / 18.0;
    pages.buildLayout(
        {.position = {.x = static_cast<float>(width / 2.0),
                      .y = static_cast<float>((buttonHeight / 2) + topMargin)},
         .halfSize = {.x = static_cast<float>((width - pageMargin) / 2.0),
                      .y = static_cast<float>(buttonHeight / 2.0)}});
    shape = {.position = {0, 0},
             .halfSize = {width / 2, (topMargin + bottomMargin + buttonHeight +
                                      seperatorHeight) /
                                         2}};
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}

  inline void handleMouseMove(const vec2f_t &mousePosition) {}

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoClickRadioGroup(&pages, mousePosition)) {

      navigation->page = NavigationPages[pages.selectedIndex];
    };
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {}

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    DrawRadioGroup(pages, renderer, style);
    auto seperatorRect = SDL_Rect{
        .x = static_cast<int>(pageMargin),
        .y = static_cast<int>(buttonHeight + topMargin + bottomMargin / 2),
        .w = static_cast<int>(pages.shape.halfSize.x * 2 - pageMargin),
        .h = static_cast<int>(seperatorHeight)};
    SDL_SetRenderDrawColor(renderer, style.inactiveColor.r,
                           style.inactiveColor.g, style.inactiveColor.b,
                           style.inactiveColor.a);
    SDL_RenderFillRect(renderer, &seperatorRect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  }
};

inline const std::string GetNoteName(int note) {
  note = note % 12;
  switch (note) {
  case 0:
    return "c";
    break;
  case 1:
    return "c#";
    break;
  case 2:
    return "d";
    break;
  case 3:
    return "d#";
    break;
  case 4:
    return "e";
    break;
  case 5:
    return "f";
    break;
  case 6:
    return "f#";
    break;
  case 7:
    return "g";
    break;
  case 8:
    return "g#";
    break;
  case 9:
    return "a";
    break;
  case 10:
    return "a#";
    break;
  case 11:
    return "b";
    break;
  }
  return "";
}

struct KeyboardUI {

  static constexpr size_t SYNTH_SELECTED_RADIO_GROUP_SIZE = 3;
  static constexpr size_t NUM_KEY_BUTTONS = 24 + 11;

  float notes[NUM_KEY_BUTTONS] = {
      36,         37,         38,         39,         40,         36 + 5,
      37 + 5,     38 + 5,     39 + 5,     40 + 5,     36 + 2 * 5, 37 + 2 * 5,
      38 + 2 * 5, 39 + 2 * 5, 40 + 2 * 5, 36 + 3 * 5, 37 + 3 * 5, 38 + 3 * 5,
      39 + 3 * 5, 40 + 3 * 5, 36 + 4 * 5, 37 + 4 * 5, 38 + 4 * 5, 39 + 4 * 5,
      40 + 4 * 5, 36 + 5 * 5, 37 + 5 * 5, 38 + 5 * 5, 39 + 5 * 5, 40 + 5 * 5,
      36 + 6 * 5, 37 + 6 * 5, 38 + 6 * 5, 39 + 6 * 5, 40 + 6 * 5,
  };
  NavigationUI *navigationUI = NULL;
  // Navigation *navigation = NULL;
  Synthesizer<float> *synth = NULL;

  std::map<SDL_FingerID, int> heldKeys;
  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  // Button mappingMenuButton;
  //  Button soundEditMenuButton;
  Button keyButtons[NUM_KEY_BUTTONS];

  float synthSelectWidth = 150;
  float synthSelectHeight = 50;
  float titleBarHeight = 100;
  float buttonMargin = 5;
  float topMargin = 15;

  static inline const KeyboardUI MakeKeyboardUI(NavigationUI *navUI,
                                                Synthesizer<float> *synth) {
    return KeyboardUI{.navigationUI = navUI, .synth = synth};
  }

  void buildLayout(const float width, const float height) {
    auto pageMargin = 50;
    auto radiobuttonMargin = 10;
    synthSelectWidth = width / 6;
    synthSelectHeight = width / 8.0;

    titleBarHeight = navigationUI->shape.halfSize.y * 2 + topMargin;
    // titleBarHeight = height / 24.0;
    // navigationUI->pages.shape = {.position{.x = 0, .y = 0},
    //.halfSize = {.x = width, .y = titleBarHeight }
    //};

    //  soundEditMenuButton = Button{
    //      .labelText = "sound edit",
    //      .shape = AxisAlignedBoundingBox{
    //          .position = vec2f_t{.x = static_cast<float>(pageMargin +
    //                                                      synthSelectWidth /
    //                                                      2),
    //                              .y = static_cast<float>(pageMargin +
    //                                                      synthSelectHeight /
    //                                                      2)},
    //          .halfSize =
    //              vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
    //                      .y = static_cast<float>(synthSelectHeight / 2)}}};
    //  mappingMenuButton = Button{
    //      .labelText = "sensor mapping",
    //      .shape = AxisAlignedBoundingBox{
    //          .position = vec2f_t{.x = width - synthSelectWidth / 2 -
    //          pageMargin,
    //                              .y = static_cast<float>(pageMargin +
    //                                                      synthSelectHeight /
    //                                                      2)},
    //          .halfSize =
    //              vec2f_t{.x = static_cast<float>(synthSelectWidth / 2),
    //                      .y = static_cast<float>(synthSelectHeight / 2)}}};

    // auto topBarHeight = synthSelectHeight + (1.5 * buttonMargin) +
    // pageMargin;
    //  auto keySize = width / 4.5;
    auto keySize = width / 5.5;
    auto keyboardStartPositionX = 100;
    float keysPerRow = 5;

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {

      keyButtons[i] = Button{
          .labelText = GetNoteName(notes[i]),
          .shape = AxisAlignedBoundingBox{
              .position =
                  vec2f_t{
                      .x = static_cast<float>(pageMargin + keySize / 2.0 +
                                              (i % int(keysPerRow)) *
                                                  (keySize + buttonMargin)),
                      .y = static_cast<float>(titleBarHeight + keySize / 2.0 +
                                              floor(i / keysPerRow) *
                                                  (keySize + buttonMargin))},
              .halfSize = vec2f_t{.x = static_cast<float>(keySize / 2),
                                  .y = static_cast<float>(keySize / 2)}}};
    }
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      if (DoButtonHover(&keyButtons[i], position)) {
        auto bentNote = notes[heldKeys[fingerId]];
        auto bendDestination = notes[i];
        synth->bendNote(bentNote, bendDestination);
        fingerPositions[fingerId] = i;
      }
    }

    for (size_t i = 0; i < NUM_KEY_BUTTONS; ++i) {
      for (size_t j = 0; j < 10; ++j) {
        if ((fingerPositions[j] == i) && (keyButtons[i].state != ACTIVE)) {
          keyButtons[i].state = HOVER;
        }
      }
    }
    //  DoButtonHover(&soundEditMenuButton, position);
    //  DoButtonHover(&mappingMenuButton, position);
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

    if (fingerPositions[fingerId] > -1) {
      keyButtons[fingerPositions[fingerId]].state = INACTIVE;
    }

    if (heldKeys.size() == 0) {
      for (auto &button : keyButtons) {
        button.state = UIState::INACTIVE;
      }
    }
    fingerPositions[fingerId] = -1;
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {}

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    navigationUI->handleMouseDown(mousePosition);
    //  if (DoButtonClick(&soundEditMenuButton, mousePosition)) {
    //    navigation->page = Navigation::SOUND_EDIT;
    //  };

    //  if (DoButtonClick(&mappingMenuButton, mousePosition)) {
    //    navigation->page = Navigation::MAPPING;
    //  };

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

    // mappingMenuButton.state = INACTIVE;

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
    navigationUI->draw(renderer, style);
    //   DrawButton(soundEditMenuButton, renderer, style);
    //   DrawButton(mappingMenuButton, renderer, style);

    for (size_t i = 0; i < NUM_KEY_BUTTONS; i++) {
      DrawButton(keyButtons[i], renderer, style);
    }
  }
};
struct MappingUI {

  NavigationUI *navigationUI = NULL;
  SensorMapping<float> *sensorMapping = NULL;
  // Button backButton;
  MultiSelectMenu sensorMenus[NUM_SENSOR_TYPES];
  std::string titleLabel = "map the phone sensors to sound parameters: ";
  std::string sensorLabels[NUM_SENSOR_TYPES];
  // Navigation *navigation = NULL;

  float titleBarHeight = 100;
  float sideMargin = 15;
  float topMargin = 50;
  float buttonMargin = 50;
  float sensorLabelWidth = 150;

  static inline const MappingUI MakeMappingUI(NavigationUI *navUI,
                                              SensorMapping<float> *mapping) {
    return MappingUI{.navigationUI = navUI, .sensorMapping = mapping};
  }

  void buildLayout(const float width, const float height) {

    titleBarHeight = navigationUI->shape.halfSize.y * 2;

    auto backButtonSize = vec2f_t{
        .x = static_cast<float>(width / 8.0),
        .y = static_cast<float>(width / 16.0),
    };
    //  backButton = Button{
    //      .labelText = "<- back",
    //      .shape = AxisAlignedBoundingBox{
    //          .position = {.x = static_cast<float>(backButtonSize.x / 2.0),
    //                       .y = static_cast<float>(backButtonSize.y / 2.0)},
    //          .halfSize = backButtonSize.scale(0.5)}};

    std::vector<Button> options;
    for (size_t i = 0; i < NUM_PARAMETER_TYPES; ++i) {
      options.push_back(Button{.labelText = ParameterTypeDisplayNames[i]});
    }

    auto menuButtonSize = vec2f_t{.x = (width - (2 * sideMargin)) / 3,
                                  .y = static_cast<float>(100)};
    auto menuButtonHalfSize = menuButtonSize.scale(0.5);
    for (auto &sensorType : SensorTypes) {
      std::stringstream formatter = std::stringstream();
      formatter << SensorTypesDisplayNames[sensorType] << " --> ";
      sensorLabels[sensorType] = formatter.str();

      sensorMenus[sensorType] = MultiSelectMenu{
          .selected = sensorMapping->mapping[sensorType],
          .menuButton =
              Button{.labelText =
                         ParameterTypeDisplayNames[sensorMapping
                                                       ->mapping[sensorType]],
                     .shape =
                         AxisAlignedBoundingBox{
                             .position = {.x = static_cast<float>(
                                              sensorLabelWidth +
                                              menuButtonSize.x + sideMargin),
                                          .y = (sensorType * (menuButtonSize.y +
                                                              buttonMargin)) +
                                               menuButtonHalfSize.y +
                                               buttonMargin + topMargin +
                                               titleBarHeight},
                             .halfSize = menuButtonHalfSize}},
          .options = options,
      };
      sensorMenus[sensorType].buildLayout(width, height);
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {

    //   DoButtonHover(&backButton, mousePosition);
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
        sensorMapping->removeMapping(sensorType,
                                     ParameterTypes[previousSelection]);
        sensorMapping->addMapping(sensorType, ParameterTypes[menu.selected]);
        menu.menuButton.labelText = ParameterTypeDisplayNames[menu.selected];
        SDL_Log("mapping changed");
        for (auto &pair : sensorMapping->mapping) {
          SDL_Log("%s -> %s", SensorTypesDisplayNames[pair.first],
                  ParameterTypeDisplayNames[pair.second]);
        }
        break;
      case MultiSelectMenu::NOTHING:
        break;
      }
    }

    if (!anyMenusActive) {
      //  if (!anyMenusActive && DoButtonClick(&backButton, mousePosition)) {
      navigationUI->handleMouseDown(mousePosition);
      //    navigation->page = Navigation::KEYBOARD;
    }
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {
    //  backButton.state = INACTIVE;
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    navigationUI->draw(renderer, style);

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
      //   DrawButton(backButton, renderer, style);
      DrawLabel(titleLabel, style.inactiveColor, style.hoverColor,
                {.x = static_cast<int>(sideMargin),
                 .y = static_cast<int>(titleBarHeight),
                 .w = 300,
                 .h = static_cast<int>(titleBarHeight)},
                renderer, style);
      for (size_t i = 0; i < NUM_SENSOR_TYPES; ++i) {
        DrawMultiSelectMenu(sensorMenus[i], renderer, style);
        DrawLabel(sensorLabels[i], style.hoverColor, style.inactiveColor,
                  SDL_Rect{.x = static_cast<int>(
                               sensorMenus[i].menuButton.shape.position.x -
                               sensorMenus[i].menuButton.shape.halfSize.x -
                               sensorLabelWidth),
                           .y = static_cast<int>(
                               sensorMenus[i].menuButton.shape.position.y -
                               sensorMenus[i].menuButton.shape.halfSize.y),
                           .w = static_cast<int>(sensorLabelWidth),
                           .h = 100},
                  renderer, style);
      }
    }
  }
};

struct SoundEditUI {

  NavigationUI *navigationUI;
  // Navigation *navigation = NULL;
  Synthesizer<float> *synth = NULL;
  SensorMapping<float> *sensorMapping = NULL;

  // Button backButton;
  RadioGroup synthSelectRadioGroup;
  HSlider parameterSliders[NUM_PARAMETER_TYPES];
  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  float titleBarHeight = 100;
  float synthSelectWidth = 50;
  float synthSelectHeight = 50;
  float buttonMargin = 5;
  float topMargin = 50;
  float pageMargin = 50;

  static inline const SoundEditUI
  MakeSoundEditUI(NavigationUI *navUI, Synthesizer<float> *synth,
                  SensorMapping<float> *mapping) {
    const size_t initialSynthTypeSelection = synth->type;
    std::vector<std::string> synthOptionLabels = {};
    for (auto &synthType : SynthTypes) {
      synthOptionLabels.push_back(SynthTypeDisplayNames[synthType]);
    }
    return SoundEditUI{
        .navigationUI = navUI,
        .synth = synth,
        .sensorMapping = mapping,
        .synthSelectRadioGroup = RadioGroup::MakeRadioGroup(
            synthOptionLabels, initialSynthTypeSelection),
    };
  }

  void buildLayout(const float width, const float height) {

    titleBarHeight = navigationUI->shape.halfSize.y * 2;

    auto radiobuttonMargin = 10;

    auto backButtonSize = vec2f_t{
        .x = static_cast<float>(width / 8.0),
        .y = static_cast<float>(width / 16.0),
    };

    synthSelectWidth =
        (width - (2 * pageMargin) - (NUM_SYNTH_TYPES * radiobuttonMargin)) /
        float(NUM_SYNTH_TYPES);
    synthSelectHeight = width / 8.0;

    synthSelectRadioGroup.buildLayout(AxisAlignedBoundingBox{

        .position = {.x = width / 2,
                     .y = static_cast<float>(topMargin + titleBarHeight +
                                             height / 24.0)},
        .halfSize = {.x = (width - 2 * pageMargin) / 2,
                     .y = static_cast<float>(height / 24.0)}});

    for (auto &parameter : ParameterTypes) {
      parameterSliders[parameter] = HSlider{
          .labelText = ParameterTypeDisplayNames[parameter],
          .value = synth->getParameter(parameter),
          .shape =
              AxisAlignedBoundingBox{
                  .position =
                      {
                          .x = width / 2,
                          .y = synthSelectRadioGroup.shape.position.y +
                               synthSelectRadioGroup.shape.halfSize.y +
                               buttonMargin +
                               (parameter + 1) *
                                   (static_cast<float>(height / 12.0) +
                                    buttonMargin),
                      },
                  .halfSize = {.x = (width - 2 * pageMargin) / 2,
                               .y = static_cast<float>(height / 24.0)}},
      };
    }
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    DoRadioGroupHover(&synthSelectRadioGroup, position);
    if (fingerPositions[fingerId] > -1) {
      auto parameterType = ParameterTypes[fingerPositions[fingerId]];
      if (DoHSliderDrag(&parameterSliders[parameterType], position)) {
        synth->pushParameterChangeEvent(parameterType,
                                        parameterSliders[parameterType].value);
      }
    }
    //   for (auto &parameterType : ParameterTypes) {

    //     if (DoHSliderDrag(&parameterSliders[parameterType], position)) {
    //       synth->pushParameterChangeEvent(parameterType,
    //                                       parameterSliders[parameterType].value);
    //     }
    //   }
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    if (DoClickRadioGroup(&synthSelectRadioGroup, position)) {
      synth->setSynthType(SynthTypes[synthSelectRadioGroup.selectedIndex]);
      synth->note(36, 100);
    };
    for (size_t i = 0; i < NUM_PARAMETER_TYPES; ++i) {
      auto &parameterType = ParameterTypes[i];
      if (DoHSliderClick(&parameterSliders[parameterType], position)) {
        fingerPositions[fingerId] = i;
        synth->pushParameterChangeEvent(parameterType,
                                        parameterSliders[parameterType].value);
      }
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    if (fingerPositions[fingerId] > -1) {
      parameterSliders[fingerPositions[fingerId]].state = INACTIVE;
    }
    fingerPositions[fingerId] = -1;
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    DoRadioGroupHover(&synthSelectRadioGroup, mousePosition);

    for (auto &parameterType : ParameterTypes) {
      if (DoHSliderDrag(&parameterSliders[parameterType], mousePosition)) {
        synth->pushParameterChangeEvent(parameterType,
                                        parameterSliders[parameterType].value);
      }
    }
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    navigationUI->handleMouseDown(mousePosition);

    if (DoClickRadioGroup(&synthSelectRadioGroup, mousePosition)) {
      synth->setSynthType(SynthTypes[synthSelectRadioGroup.selectedIndex]);
      synth->note(36, 100);
    };

    //  if (DoButtonClick(&backButton, mousePosition, ACTIVE)) {
    //    navigation->page = Navigation::KEYBOARD;
    //  };

    // for (auto &parameterType : ParameterTypes) {
    //   if (DoHSliderClick(&parameterSliders[parameterType], mousePosition)) {
    //     synth->pushParameterChangeEvent(parameterType,
    //                                     parameterSliders[parameterType].value);
    //   }
    // }
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {
    for (auto &parameterType : ParameterTypes) {
      parameterSliders[parameterType].state = INACTIVE;
    }

    synth->note(36, 0);
    synth->note(36, 0);
    // backButton.state = INACTIVE;
  }

  void draw(SDL_Renderer *renderer, const Style &style) {
    navigationUI->draw(renderer, style);
    // DrawButton(backButton, renderer, style);
    //
    DrawRadioGroup(synthSelectRadioGroup, renderer, style);
    for (auto &parameterType : ParameterTypes) {
      if (sensorMapping->isMapped(parameterType)) {
        auto rect = ConvertAxisAlignedBoxToSDL_Rect(
            parameterSliders[parameterType].shape);
        SDL_SetRenderDrawColor(
            renderer, style.unavailableColor.r, style.unavailableColor.g,
            style.unavailableColor.b, style.unavailableColor.a);
        SDL_RenderFillRect(renderer, &rect);
        auto percentRect = rect;
        percentRect.w *= synth->getParameter(parameterType);
        SDL_SetRenderDrawColor(renderer, style.hoverColor.r, style.hoverColor.g,
                               style.hoverColor.b, style.hoverColor.a);
        SDL_RenderFillRect(renderer, &percentRect);
        DrawLabel(parameterSliders[parameterType].labelText, style.hoverColor,
                  style.unavailableColor, rect, renderer, style);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      } else {

        DrawHSlider(parameterSliders[parameterType], renderer, style);
      }
    }
  }
};

struct UserInterface {
  Navigation navigation;

  NavigationUI navigationUI;
  KeyboardUI keyboardUI;
  MappingUI mappingUI;
  SoundEditUI soundEditUI;
  UserInterface(Synthesizer<float> *synth,
                SensorMapping<float> *sensorMapping) {

    navigationUI = NavigationUI::MakeNavigationUI(&navigation);
    keyboardUI = KeyboardUI::MakeKeyboardUI(&navigationUI, synth);
    mappingUI = MappingUI::MakeMappingUI(&navigationUI, sensorMapping);
    soundEditUI =
        SoundEditUI::MakeSoundEditUI(&navigationUI, synth, sensorMapping);
  }

  inline void buildLayout(const float width, const float height) {

    navigationUI.buildLayout(width, height);
    mappingUI.buildLayout(width, height);
    keyboardUI.buildLayout(width, height);
    soundEditUI.buildLayout(width, height);
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {

    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.handleFingerMove(fingerId, position, pressure);
      break;
    case Navigation::MAPPING:
      mappingUI.handleFingerMove(fingerId, position, pressure);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleFingerMove(fingerId, position, pressure);
      break;
    }
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {

    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.handleFingerDown(fingerId, position, pressure);
      break;
    case Navigation::MAPPING:
      mappingUI.handleFingerDown(fingerId, position, pressure);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleFingerDown(fingerId, position, pressure);
      break;
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.handleFingerUp(fingerId, position, pressure);
      break;
    case Navigation::MAPPING:
      mappingUI.handleFingerUp(fingerId, position, pressure);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleFingerUp(fingerId, position, pressure);
      break;
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.handleMouseMove(mousePosition);
      break;
    case Navigation::MAPPING:
      mappingUI.handleMouseMove(mousePosition);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleMouseMove(mousePosition);
      break;
    }
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.handleMouseDown(mousePosition);
      break;
    case Navigation::MAPPING:
      mappingUI.handleMouseDown(mousePosition);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleMouseDown(mousePosition);
      break;
    }
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {
    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.handleMouseUp(mousePosition);
      break;
    case Navigation::MAPPING:
      mappingUI.handleMouseUp(mousePosition);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.handleMouseUp(mousePosition);
      break;
    }
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {
    switch (navigation.page) {
    case Navigation::KEYBOARD:
      keyboardUI.draw(renderer, style);
      break;
    case Navigation::MAPPING:
      mappingUI.draw(renderer, style);
      break;
    case Navigation::SOUND_EDIT:
      soundEditUI.draw(renderer, style);
      break;
    }
  }
};
