#pragma once

#include "sensor.h"
#include "ui_navigation.h"
#include <sstream>
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
