#pragma once

#include "SDL_render.h"
#include "collider.h"
#include "mapping.h"
#include "metaphor.h"
#include "save_state.h"
#include "synthesis.h"
#include "synthesis_parameter.h"
#include "ui_navigation.h"
#include "vector_math.h"
#include "widget_button.h"
#include <sstream>
#include <vector>

struct MappingUI {
  SaveState *saveState = NULL;
  Label pageTitleLabel = Label("choose your abilities!");
  Label momentaryInputLabel = Label("momentary inputs");
  Label continuousInputLabel = Label("continuous inputs");

  std::map<MomentaryInputType, Button> momentaryInputButtons =
      std::map<MomentaryInputType, Button>();
  std::map<ContinuousInputType, Button> continuousInputButtons =
      std::map<ContinuousInputType, Button>();

  Button parameterButtons[NUM_PARAMETER_TYPES];
  Button momentaryParameterButtons[NUM_MOMENTARY_PARAMETER_TYPES];

  AxisAlignedBoundingBox shape;
  AxisAlignedBoundingBox bodyShape;

  int heldSensor = -1;
  int heldMomentarySensor = -1;
  vec2f_t lastMousePosition;

  MappingUI(SaveState *_saveState) : saveState(_saveState) {}

  ~MappingUI() {}

  void refreshLayout() { buildLayout(shape); }
  void buildLayout(const AxisAlignedBoundingBox &shape) {
    momentaryInputButtons.clear();
    continuousInputButtons.clear();
    std::vector<MomentaryInputType> momentaryInputTypes;
    std::vector<ContinuousInputType> continuousInputTypes;
    getMomentaryInputsForInstrumentType(saveState->getInstrumentMetaphorType(),
                                        &momentaryInputTypes);
    getContinuousInputsForInstrumentType(saveState->getInstrumentMetaphorType(),
                                         &continuousInputTypes);

    auto maxNumButtons =
        std::max(momentaryInputTypes.size() + continuousInputTypes.size() +
                     NUM_SENSOR_INPUT_TYPES,
                 NUM_PARAMETER_TYPES + 2);
    this->shape = shape;

    auto pageLabelHeight = shape.halfSize.y / 24;

    auto topPosition = shape.position.y - shape.halfSize.y;
    pageTitleLabel.shape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = topPosition +
                          static_cast<float>(pageLabelHeight / 2.0)},
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
    float sideMargin = bodyShape.halfSize.x / 10;
    float topMargin = bodyShape.halfSize.y / 10;
    float buttonMargin = bodyShape.halfSize.y / 32;
    vec2f_t buttonHalfSize = {.x = bodyShape.halfSize.x / 3,
                              .y = (bodyShape.halfSize.y - pageLabelHeight / 2 -
                                    (buttonMargin * maxNumButtons)) /
                                   maxNumButtons};
    auto buttonSpace = buttonMargin + buttonHalfSize.y * 2;

    auto yPos = yOffset + 5;

    momentaryInputLabel.shape = AxisAlignedBoundingBox{
        .position = {.x = xOffset + bodyShape.halfSize.x / 3 + sideMargin,
                     .y = yPos + static_cast<float>(pageLabelHeight +
                                                    buttonMargin / 2)},
        .halfSize = {.x = bodyShape.halfSize.x / 3,
                     .y = static_cast<float>(pageLabelHeight / 2.0)}};
    yPos = momentaryInputLabel.shape.position.y +
           momentaryInputLabel.shape.halfSize.y + buttonHalfSize.y +
           buttonMargin;

    for (auto inputType : momentaryInputTypes) {
      momentaryInputButtons[inputType] =
          Button{.label = Label(getDisplayName(inputType)),
                 .shape = AxisAlignedBoundingBox{
                     .position = {.x = buttonHalfSize.x + xOffset, .y = yPos},
                     .halfSize = buttonHalfSize}};
      yPos += buttonSpace;
    }

    continuousInputLabel.shape = AxisAlignedBoundingBox{
        .position = {.x = xOffset + bodyShape.halfSize.x / 3 + sideMargin,
                     .y = yPos + static_cast<float>(pageLabelHeight +
                                                    buttonMargin / 2)},
        .halfSize = {.x = bodyShape.halfSize.x / 3,
                     .y = static_cast<float>(pageLabelHeight / 2.0)}};
    yPos = continuousInputLabel.shape.position.y +
           continuousInputLabel.shape.halfSize.y + buttonHalfSize.y +
           buttonMargin;

    for (auto &sensorType : continuousInputTypes) {
      continuousInputButtons[sensorType] =
          Button{.label = Label(getDisplayName(sensorType)),
                 .shape = AxisAlignedBoundingBox{
                     .position = {.x = buttonHalfSize.x + xOffset, .y = yPos},
                     .halfSize = buttonHalfSize}};
      yPos += buttonSpace;
    }

    for (auto &sensorType : SensorInputTypes) {
      continuousInputButtons[sensorType] =
          Button{.label = Label(getDisplayName(sensorType)),
                 .shape = AxisAlignedBoundingBox{
                     .position = {.x = buttonHalfSize.x + xOffset, .y = yPos},
                     .halfSize = buttonHalfSize}};
      yPos += buttonSpace;
    }

    yPos = momentaryInputLabel.shape.position.y +
           momentaryInputLabel.shape.halfSize.y + buttonHalfSize.y +
           buttonMargin;

    for (auto &parameterType : MomentaryParameterTypes) {
      momentaryParameterButtons[parameterType] = Button{
          .label = Label(getDisplayName(parameterType)),
          .shape = AxisAlignedBoundingBox{
              .position = {.x = width - buttonHalfSize.x + xOffset, .y = yPos},
              .halfSize = buttonHalfSize}};
      yPos += buttonSpace;
    }

    yPos = continuousInputLabel.shape.position.y +
           continuousInputLabel.shape.halfSize.y + buttonHalfSize.y +
           buttonMargin;

    for (auto &parameterType : ParameterTypes) {
      parameterButtons[parameterType] = Button{
          .label = Label(getDisplayName(parameterType)),
          .shape = AxisAlignedBoundingBox{
              .position = {.x = width - buttonHalfSize.x + xOffset, .y = yPos},
              .halfSize = buttonHalfSize}};
      yPos += buttonSpace;
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
      if ((continuousInputButtons.find(sensorType) !=
           continuousInputButtons.end()) &&
          DoButtonClick(&continuousInputButtons[sensorType], mousePosition)) {
        heldSensor = sensorType;
      }
    }

    for (auto &parameterType : ParameterTypes) {
      if (DoButtonClick(&parameterButtons[parameterType], mousePosition)) {

        saveState->sensorMapping.removeMappingForParameterType(parameterType);
      }
    }
    for (auto &parameterType : MomentaryParameterTypes) {
      if (DoButtonClick(&momentaryParameterButtons[parameterType],
                        mousePosition)) {

        saveState->sensorMapping.removeMappingForParameterType(parameterType);
      }
    }
  }
  inline void handleMouseUp(const vec2f_t &mousePosition) {

    lastMousePosition = mousePosition;
    for (auto &parameterType : ParameterTypes) {
      if ((heldSensor > -1)) {
        if (DoButtonClick(&parameterButtons[parameterType], mousePosition)) {

          saveState->sensorMapping.addMapping((ContinuousInputType)heldSensor,
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

          saveState->sensorMapping.addMapping(
              (MomentaryInputType)heldMomentarySensor, parameterType);
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

  inline void drawConnectionLine(SDL_Renderer *renderer,
                                 const vec2f_t &position1,
                                 const vec2f_t &position2) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderDrawLine(renderer, position1.x, position1.y, position2.x,
                       position2.y);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  }
  inline void drawConnectionLine(SDL_Renderer *renderer, const Button &button1,
                                 const Button &button2) {
    drawConnectionLine(
        renderer,
        vec2f_t{.x = button1.shape.position.x + button1.shape.halfSize.x,
                .y = button1.shape.position.y},
        vec2f_t{.x = button2.shape.position.x - button2.shape.halfSize.x,
                .y = button2.shape.position.y});
  }

  inline void draw(SDL_Renderer *renderer, const Style &style) {

    for (auto &keyValuePair : saveState->sensorMapping.continuousMappings) {
      auto &sensorButton = continuousInputButtons[keyValuePair.second];
      auto &parameterButton = parameterButtons[keyValuePair.first];
      drawConnectionLine(renderer, sensorButton, parameterButton);
    }
    for (auto &keyValuePair : saveState->sensorMapping.momentaryMappings) {
      auto &sensorButton = momentaryInputButtons[keyValuePair.second];
      auto &parameterButton = momentaryParameterButtons[keyValuePair.first];
      drawConnectionLine(renderer, sensorButton, parameterButton);
    }
    if (heldSensor > -1) {
      auto &sensorButton =
          continuousInputButtons[(ContinuousInputType)heldSensor];

      drawConnectionLine(renderer, sensorButton.shape.position,
                         lastMousePosition);
    }
    if (heldMomentarySensor > -1) {
      auto &sensorButton =
          momentaryInputButtons[(MomentaryInputType)heldMomentarySensor];
      drawConnectionLine(renderer, sensorButton.shape.position,
                         lastMousePosition);
    };

    for (auto &typeButtonPair : momentaryInputButtons) {
      DrawButton(&typeButtonPair.second, renderer, style, style.color1);
    }
    for (auto &typeButtonPair : continuousInputButtons) {
      DrawButton(&typeButtonPair.second, renderer, style, style.color2);
    }
    for (auto &button : momentaryParameterButtons) {
      DrawButton(&button, renderer, style, style.color1);
    }
    for (auto &button : parameterButtons) {
      DrawButton(&button, renderer, style, style.color2);
    }

    pageTitleLabel.draw(style.hoverColor, style.unavailableColor, renderer,
                        style, HorizontalAlignment::CENTER,
                        VerticalAlignment::CENTER);

    momentaryInputLabel.draw(style.hoverColor, style.unavailableColor, renderer,
                             style, HorizontalAlignment::LEFT,
                             VerticalAlignment::CENTER);

    continuousInputLabel.draw(style.hoverColor, style.unavailableColor,
                              renderer, style, HorizontalAlignment::LEFT,
                              VerticalAlignment::CENTER);
  }
};
