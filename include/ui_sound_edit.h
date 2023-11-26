#pragma once

#include "sensor.h"
#include "synthesis.h"
#include "ui_navigation.h"

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
