#pragma once

#include "collider.h"
#include "save_state.h"
#include "sensor.h"
#include "synthesis.h"
#include "synthesis_parameter.h"
#include "ui_navigation.h"
#include "widget_style.h"

struct SoundEditUI {
  Synthesizer<float> *synth = NULL;
  InputMapping<float> *mapping = NULL;
  SaveState *saveState = NULL;

  RadioGroup synthSelectRadioGroup;
  std::map<ContinuousParameterType, HSlider> parameterSliders;
  std::map<ContinuousParameterType, Button> mappingButtons;
  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  Label *pageTitleLabel = new Label("choose your sound!");
  AxisAlignedBoundingBox pageTitleLabelShape;
  AxisAlignedBoundingBox bodyShape;

  float synthSelectWidth = 50;
  float synthSelectHeight = 50;
  float buttonMargin = 5;
  float topMargin = 50;
  float pageMargin = 50;

  SoundEditUI(Synthesizer<float> *_synth, InputMapping<float> *_mapping,
              SaveState *_saveState)
      : synth(_synth), mapping(_mapping), saveState(_saveState) {
    const size_t initialSynthTypeSelection = synth->type;
    std::vector<std::string> synthOptionLabels = {};
    for (auto &synthType : SynthTypes) {
      synthOptionLabels.push_back(SynthTypeDisplayNames[synthType]);
    }
    synthSelectRadioGroup =
        RadioGroup(synthOptionLabels, saveState->synthesizerSettings.synthType);
    synthSelectRadioGroup.options[SUBTRACTIVE_DRUM_SYNTH].iconType =
        IconType::DRUM;
    synthSelectRadioGroup.options[SUBTRACTIVE].iconType = IconType::SINE_WAVE;
    synthSelectRadioGroup.options[PHYSICAL_MODEL].iconType = IconType::GUITAR;
    synthSelectRadioGroup.options[FREQUENCY_MODULATION].iconType =
        IconType::GRAPH;
    synthSelectRadioGroup.options[SAMPLER].iconType = IconType::CASSETTE;
  }

  ~SoundEditUI() { delete pageTitleLabel; }

  void buildLayout(const AxisAlignedBoundingBox &shape) {

    auto pageLabelHeight = 50;
    pageTitleLabelShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = shape.position.y - shape.halfSize.y +
                          static_cast<float>(pageLabelHeight / 2.0)},
        .halfSize = {.x = shape.halfSize.x,
                     .y = static_cast<float>(pageLabelHeight / 2.0)}};
    bodyShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = shape.position.y + pageLabelHeight},
        .halfSize = {
            .x = shape.halfSize.x,
            .y = static_cast<float>(shape.halfSize.y - pageLabelHeight / 2.0)}};
    auto width = bodyShape.halfSize.x * 2;
    auto height = bodyShape.halfSize.y * 2;
    auto xOffset = bodyShape.position.x - bodyShape.halfSize.x;
    auto yOffset = bodyShape.position.y - bodyShape.halfSize.y;

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

        .position = {.x = shape.halfSize.x + xOffset,
                     .y = static_cast<float>(height / 24.0 + yOffset)},
        .halfSize = {.x = (width - 2 * pageMargin) / 2,
                     .y = static_cast<float>(height / 24.0)}});

    int sliderCounter = 0;
    auto initialSliderY = synthSelectRadioGroup.shape.position.y +
                          2 * synthSelectRadioGroup.shape.halfSize.y +
                          buttonMargin * 2;
    auto usefulWidth = width - pageMargin;
    auto rowMargin = usefulWidth / 128;
    auto buttonWidth = 0 * (usefulWidth - rowMargin) / 8;
    auto sliderWidth = usefulWidth - rowMargin - buttonWidth;
    auto rowHeight = height / 12.0;
    for (auto &parameter : ParameterTypes) {
      if (parameter == FREQUENCY)
        continue;
      auto rowY =
          initialSliderY +
          sliderCounter++ * (static_cast<float>(height / 12.0) + buttonMargin);
      parameterSliders[parameter] = HSlider{
          .label = Label(getDisplayName(parameter)),
          .shape =
              AxisAlignedBoundingBox{
                  .position =
                      {
                          .x = xOffset + pageMargin / 2 + sliderWidth / 2,
                          .y = rowY,
                      },
                  .halfSize = {.x = sliderWidth / 2,
                               .y = static_cast<float>(rowHeight / 2)}},
      };
      ContinuousInputType mappedInput;
      auto buttonText = mapping->getMapping(parameter, &mappedInput)
                            ? getDisplayName(mappedInput)
                            : "none";
      mappingButtons[parameter] =
          Button{.label = Label(buttonText),
                 .shape = AxisAlignedBoundingBox{
                     .position = {.x = xOffset + pageMargin / 2 + sliderWidth +
                                       rowMargin + buttonWidth / 2,
                                  .y = rowY},
                     .halfSize = {.x = buttonWidth / 2,
                                  .y = static_cast<float>(rowHeight / 2)}}};
    }
  }

  inline void handleFingerMove(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {
    DoRadioGroupHover(&synthSelectRadioGroup, position);
    if (fingerPositions[fingerId] > -1) {
      auto parameterType = ParameterTypes[fingerPositions[fingerId]];
      float paramValue = synth->getParameter(parameterType);
      if (DoHSliderDrag(&parameterSliders[parameterType], &paramValue,
                        position)) {
        synth->pushParameterChangeEvent(parameterType, paramValue);
      }
    }
  }

  inline void handleFingerDown(const SDL_FingerID &fingerId,
                               const vec2f_t &position, const float pressure) {

    for (auto &parameterType : ParameterTypes) {
      if (parameterSliders.find(parameterType) == parameterSliders.end()) {
        continue;
      }
      float paramValue = synth->getParameter(parameterType);
      if (DoHSliderClick(&parameterSliders[parameterType], &paramValue,
                         position)) {
        fingerPositions[fingerId] = parameterType;
        synth->pushParameterChangeEvent(parameterType, paramValue);
      }
    }
  }

  inline void handleFingerUp(const SDL_FingerID &fingerId,
                             const vec2f_t &position, const float pressure) {
    if (fingerPositions[fingerId] > -1) {
      parameterSliders[(ContinuousParameterType)fingerPositions[fingerId]]
          .state = INACTIVE;
    }
    fingerPositions[fingerId] = -1;
  }

  inline void handleMouseMove(const vec2f_t &mousePosition) {
    DoRadioGroupHover(&synthSelectRadioGroup, mousePosition);

    for (auto &parameterType : ParameterTypes) {
      if (parameterSliders.find(parameterType) == parameterSliders.end()) {
        continue;
      }
      float paramValue = synth->getParameter(parameterType);
      if (DoHSliderDrag(&parameterSliders[parameterType], &paramValue,
                        mousePosition)) {
        synth->pushParameterChangeEvent(parameterType, paramValue);
      }
    }
  }

  inline void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoClickRadioGroup(&synthSelectRadioGroup, mousePosition)) {
      synth->setSynthType(SynthTypes[synthSelectRadioGroup.selectedIndex]);
      synth->setFrequency(mtof(36));
      synth->pushGateEvent(MomentaryParameterType::GATE, 1);
    };
  }

  inline void handleMouseUp(const vec2f_t &mousePosition) {
    for (auto &parameterType : ParameterTypes) {
      if (parameterSliders.find(parameterType) == parameterSliders.end()) {
        continue;
      }
      parameterSliders[parameterType].state = INACTIVE;
    }
    synth->setFrequency(mtof(36));
    synth->pushGateEvent(MomentaryParameterType::GATE, 0);
  }

  void draw(SDL_Renderer *renderer, const Style &style) {

    auto pageLabelRect = ConvertAxisAlignedBoxToSDL_Rect(pageTitleLabelShape);
    pageTitleLabel->draw(
        style.hoverColor, style.unavailableColor, pageLabelRect, renderer,
        style, HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
    synthSelectRadioGroup.selectedIndex = synth->getSynthType();
    DrawRadioGroup(&synthSelectRadioGroup, renderer, style);
    for (auto &parameterType : ParameterTypes) {
      if (parameterSliders.find(parameterType) == parameterSliders.end()) {
        continue;
      }
      if (saveState->sensorMapping.isMapped(parameterType)) {
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
        parameterSliders[parameterType].label.draw(
            style.hoverColor, style.unavailableColor, rect, renderer, style);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      } else {

        DrawHSlider(&parameterSliders[parameterType],
                    synth->getParameter(parameterType), renderer, style);
      }

      //  DrawButton(&mappingButtons[parameterType], renderer, style);
    }
  }
};
