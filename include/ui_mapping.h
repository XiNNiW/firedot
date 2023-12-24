#pragma once

#include "SDL_render.h"
#include "collider.h"
#include "metaphor.h"
#include "save_state.h"
#include "sensor.h"
#include "synthesis.h"
#include "synthesis_parameter.h"
#include "ui_navigation.h"
#include "vector_math.h"
#include "widget_button.h"
#include <sstream>

struct MappingUI {
  InputMapping<float> *sensorMapping = NULL;
  SaveState *saveState = NULL;
  Label *pageTitleLabel = new Label("choose your abilities!");
  std::map<MomentaryInputType, Button> momentaryInputButtons;
  std::map<ContinuousInputType, Button> sensorButtons;
  Button parameterButtons[NUM_PARAMETER_TYPES];
  Button momentaryParameterButtons[NUM_MOMENTARY_PARAMETER_TYPES];

  AxisAlignedBoundingBox pageTitleLabelShape;

  AxisAlignedBoundingBox shape;
  AxisAlignedBoundingBox bodyShape;
  float sideMargin = 15;
  float topMargin = 50;
  float buttonMargin = 50;
  int heldSensor = -1;
  int heldMomentarySensor = -1;
  vec2f_t lastMousePosition;

  MappingUI(InputMapping<float> *_mapping, SaveState *_saveState)
      : sensorMapping(_mapping), saveState(_saveState) {}

  ~MappingUI() { delete pageTitleLabel; }

  void refreshLayout() { buildLayout(shape); }
  void buildLayout(const AxisAlignedBoundingBox &shape) {
    this->shape = shape;
    auto pageLabelHeight = 50;
    auto topPosition = shape.position.y - shape.halfSize.y;
    pageTitleLabelShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = topPosition +
                          static_cast<float>(pageLabelHeight / 2.0)},
        .halfSize = {.x = shape.halfSize.x,
                     .y = topPosition +
                          static_cast<float>(pageLabelHeight / 2.0)}};
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

    auto buttonHalfSize = vec2f_t{.x = 150, .y = 75};
    auto sensorButtonSpaceY =
        height / float(NUM_CONTINUOUS_INPUT_TYPES + NUM_MOMENTARY_INPUT_TYPES);
    auto parameterButtonSpaceY =
        height / float(NUM_PARAMETER_TYPES + NUM_MOMENTARY_PARAMETER_TYPES);
    int inputButtonCounter = 0;

    for (auto inputType : MomentaryInputTypes) {
      momentaryInputButtons[inputType] = Button{
          .label = Label(getDisplayName(inputType)),
          .shape = AxisAlignedBoundingBox{
              .position = {.x = buttonHalfSize.x + xOffset,
                           .y = inputButtonCounter++ * sensorButtonSpaceY +
                                buttonHalfSize.y + yOffset},
              .halfSize = buttonHalfSize}};
    }

    for (auto &sensorType : InstrumentInputTypes) {
      if (isInstrumentInputType(saveState->instrumentMetaphor, sensorType)) {
        sensorButtons[sensorType] = Button{
            .label = Label(getDisplayName(sensorType)),
            .shape = AxisAlignedBoundingBox{
                .position = {.x = buttonHalfSize.x + xOffset,
                             .y = inputButtonCounter++ * sensorButtonSpaceY +
                                  buttonHalfSize.y + yOffset},
                .halfSize = buttonHalfSize}};
      }
    }

    for (auto &sensorType : SensorInputTypes) {
      sensorButtons[sensorType] = Button{
          .label = Label(getDisplayName(sensorType)),
          .shape = AxisAlignedBoundingBox{
              .position = {.x = buttonHalfSize.x + xOffset,
                           .y = inputButtonCounter++ * sensorButtonSpaceY +
                                buttonHalfSize.y + yOffset},
              .halfSize = buttonHalfSize}};
    }

    int outputButtonCounter = 0;
    for (auto &parameterType : MomentaryParameterTypes) {
      momentaryParameterButtons[parameterType] = Button{
          .label = Label(getDisplayName(parameterType)),
          .shape = AxisAlignedBoundingBox{
              .position = {.x = width - buttonHalfSize.x + xOffset,
                           .y = outputButtonCounter++ * parameterButtonSpaceY +
                                buttonHalfSize.y + yOffset},
              .halfSize = buttonHalfSize}};
    }

    for (auto &parameterType : ParameterTypes) {
      parameterButtons[parameterType] = Button{
          .label = Label(getDisplayName(parameterType)),
          .shape = AxisAlignedBoundingBox{
              .position = {.x = width - buttonHalfSize.x + xOffset,
                           .y = outputButtonCounter++ * parameterButtonSpaceY +
                                buttonHalfSize.y + yOffset},
              .halfSize = buttonHalfSize}};
    }
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    lastMousePosition = mousePosition;
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    bool anyMenusActive = false;
    lastMousePosition = mousePosition;

    for (auto &sensorType : MomentaryInputTypes) {
      if ((momentaryInputButtons.find(sensorType) !=
           momentaryInputButtons.end()) &&
          DoButtonClick(&momentaryInputButtons[sensorType], mousePosition)) {
        heldMomentarySensor = sensorType;
      }
    }
    for (auto &sensorType : ContinuousInputTypes) {
      if ((sensorButtons.find(sensorType) != sensorButtons.end()) &&
          DoButtonClick(&sensorButtons[sensorType], mousePosition)) {
        heldSensor = sensorType;
      }
    }

    for (auto &parameterType : ParameterTypes) {
      if (DoButtonClick(&parameterButtons[parameterType], mousePosition)) {

        sensorMapping->removeMappingForParameterType(parameterType);
      }
    }
    for (auto &parameterType : MomentaryParameterTypes) {
      if (DoButtonClick(&momentaryParameterButtons[parameterType],
                        mousePosition)) {

        sensorMapping->removeMappingForParameterType(parameterType);
      }
    }
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {

    lastMousePosition = mousePosition;
    for (auto &parameterType : ParameterTypes) {
      if ((heldSensor > -1)) {
        if (DoButtonClick(&parameterButtons[parameterType], mousePosition)) {

          sensorMapping->addMapping((ContinuousInputType)heldSensor,
                                    parameterType);
        } else {
          SDL_Log("remove mapping!");
        }
      }
    }
    for (auto &parameterType : MomentaryParameterTypes) {
      if ((heldMomentarySensor > -1)) {
        if (DoButtonClick(&momentaryParameterButtons[parameterType],
                          mousePosition)) {

          sensorMapping->addMapping((MomentaryInputType)heldMomentarySensor,
                                    parameterType);
        } else {
          SDL_Log("remove mapping!");
        }
      }
    }

    heldSensor = -1;
    heldMomentarySensor = -1;
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {}
  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {}

  inline void drawConnectionLine(SDL_Renderer *renderer, vec2f_t position1,
                                 vec2f_t position2) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderDrawLine(renderer, position1.x, position1.y, position2.x,
                       position2.y);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    auto pageLabelRect = ConvertAxisAlignedBoxToSDL_Rect(pageTitleLabelShape);
    pageTitleLabel->draw(
        style.hoverColor, style.unavailableColor, pageLabelRect, renderer,
        style, HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    for (auto &keyValuePair : sensorMapping->continuous_mappings) {
      auto sensorButton = sensorButtons[keyValuePair.first];
      auto parameterButton = parameterButtons[keyValuePair.second];
      drawConnectionLine(renderer, sensorButton.shape.position,
                         parameterButton.shape.position);
    }
    for (auto &keyValuePair : sensorMapping->momentary_mappings) {
      auto sensorButton = momentaryInputButtons[keyValuePair.first];
      auto parameterButton = momentaryParameterButtons[keyValuePair.second];
      drawConnectionLine(renderer, sensorButton.shape.position,
                         parameterButton.shape.position);
    }
    if (heldSensor > -1) {
      auto sensorButton = sensorButtons[(ContinuousInputType)heldSensor];

      drawConnectionLine(renderer, sensorButton.shape.position,
                         lastMousePosition);
    }
    if (heldMomentarySensor > -1) {
      auto sensorButton =
          momentaryInputButtons[(MomentaryInputType)heldMomentarySensor];
      drawConnectionLine(renderer, sensorButton.shape.position,
                         lastMousePosition);
    }
    auto momentaryStyle = style;
    momentaryStyle.color2 = style.color0;

    for (auto &typeButtonPair : momentaryInputButtons) {
      DrawButton(&typeButtonPair.second, renderer, momentaryStyle);
    }

    for (auto &button : momentaryParameterButtons) {
      DrawButton(&button, renderer, momentaryStyle);
    }

    for (auto &button : parameterButtons) {
      DrawButton(&button, renderer, style);
    }

    for (auto &typeButtonPair : sensorButtons) {
      DrawButton(&typeButtonPair.second, renderer, style);
    }
  }
};
