#pragma once

#include "SDL_render.h"
#include "collider.h"
#include "sensor.h"
#include "synthesis.h"
#include "ui_navigation.h"
#include "vector_math.h"
#include "widget_button.h"
#include <sstream>

struct MappingUI {
  SensorMapping<float> *sensorMapping = NULL;
  std::string titleLabel = "map the phone sensors to sound parameters: ";
  std::string sensorLabels[NUM_SENSOR_TYPES];
  Button sensorButtons[NUM_SENSOR_TYPES];
  Button parameterButtons[NUM_PARAMETER_TYPES];

  std::string pageTitleLabel = "choose your abilities";
  AxisAlignedBoundingBox pageTitleLabelShape;

  AxisAlignedBoundingBox bodyShape;
  float sideMargin = 15;
  float topMargin = 50;
  float buttonMargin = 50;
  float sensorLabelWidth = 150;
  int heldSensor = -1;
  vec2f_t lastMousePosition;

  static inline const MappingUI MakeMappingUI(SensorMapping<float> *mapping) {
    return MappingUI{.sensorMapping = mapping};
  }

  void buildLayout(const AxisAlignedBoundingBox &shape) {

    auto pageLabelHeight = 50;
    pageTitleLabelShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = static_cast<float>(pageLabelHeight / 2.0)},
        .halfSize = {.x = shape.halfSize.x,
                     .y = static_cast<float>(pageLabelHeight / 2.0)}};
    bodyShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = shape.position.y + pageLabelHeight},
        .halfSize = {
            .x = shape.halfSize.x,
            .y = static_cast<float>(shape.halfSize.y - pageLabelHeight / 2.0)}};
    auto width = (bodyShape.halfSize.x * 2);
    auto height = (bodyShape.halfSize.y * 2);
    auto xOffset = bodyShape.position.x - bodyShape.halfSize.x;
    auto yOffset = bodyShape.position.y - bodyShape.halfSize.y;

    // titleBarHeight = bodyShape.position.y + 100;
    auto buttonHalfSize = vec2f_t{.x = 150, .y = 75};
    //  auto uiHeight = (height - titleBarHeight);
    auto sensorButtonSpaceY = height / float(NUM_SENSOR_TYPES);
    auto parameterButtonSpaceY = height / float(NUM_PARAMETER_TYPES);

    for (auto &sensorType : SensorTypes) {
      sensorButtons[sensorType] =
          Button{.labelText = SensorTypesDisplayNames[sensorType],
                 .shape = AxisAlignedBoundingBox{
                     .position = {.x = buttonHalfSize.x + xOffset,
                                  .y = sensorType * sensorButtonSpaceY +
                                       buttonHalfSize.y + yOffset},
                     .halfSize = buttonHalfSize}};
    }

    for (auto &parameterType : ParameterTypes) {
      parameterButtons[parameterType] =
          Button{.labelText = ParameterTypeDisplayNames[parameterType],
                 .shape = AxisAlignedBoundingBox{
                     .position = {.x = width - buttonHalfSize.x + xOffset,
                                  .y = parameterType * parameterButtonSpaceY +
                                       buttonHalfSize.y + yOffset},
                     .halfSize = buttonHalfSize}};
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    lastMousePosition = mousePosition;
    //   DoButtonHover(&backButton, mousePosition);
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    // navigationUI->handleMouseDown(mousePosition);
    bool anyMenusActive = false;
    lastMousePosition = mousePosition;

    for (auto &sensorType : SensorTypes) {
      if (DoButtonClick(&sensorButtons[sensorType], mousePosition)) {
        heldSensor = sensorType;
      }
    }

    for (auto &parameterType : ParameterTypes) {
      if (DoButtonClick(&parameterButtons[parameterType], mousePosition)) {
        bool shouldRemove = false;
        SensorType sensorType;
        for (auto &keyValuePair : sensorMapping->mapping) {
          if (keyValuePair.second == parameterType) {
            shouldRemove = true;
            sensorType = keyValuePair.first;
          }
        }
        if (shouldRemove) {
          sensorMapping->removeMapping(sensorType, parameterType);
        }
      }
    }
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {

    lastMousePosition = mousePosition;
    //  backButton.state = INACTIVE;
    for (auto &parameterType : ParameterTypes) {
      if ((heldSensor > -1)) {
        if (DoButtonClick(&parameterButtons[parameterType], mousePosition)) {

          sensorMapping->addMapping((SensorType)heldSensor, parameterType);
        } else {
          SDL_Log("remove mapping!");
        }
      }
    }
    heldSensor = -1;
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    auto pageLabelRect = ConvertAxisAlignedBoxToSDL_Rect(pageTitleLabelShape);
    DrawLabel(pageTitleLabel, style.hoverColor, style.unavailableColor,
              pageLabelRect, renderer, style, HorizontalAlignment::CENTER,
              VerticalAlignment::CENTER);
    for (auto &keyValuePair : sensorMapping->mapping) {
      auto sensorButton = sensorButtons[keyValuePair.first];
      auto parameterButton = parameterButtons[keyValuePair.second];
      SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
      SDL_RenderDrawLine(renderer, sensorButton.shape.position.x,
                         sensorButton.shape.position.y,
                         parameterButton.shape.position.x,
                         parameterButton.shape.position.y);
      SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    }

    if (heldSensor > -1) {
      auto sensorButton = sensorButtons[heldSensor];

      SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
      SDL_RenderDrawLine(renderer, sensorButton.shape.position.x,
                         sensorButton.shape.position.y, lastMousePosition.x,
                         lastMousePosition.y);

      SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    }

    for (auto &button : parameterButtons) {
      DrawButton(button, renderer, style);
    }

    for (auto &button : sensorButtons) {
      DrawButton(button, renderer, style);
    }
  }
};
