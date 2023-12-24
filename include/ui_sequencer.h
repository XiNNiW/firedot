#pragma once

#include "sequencer.h"
#include "ui_abstract.h"
#include "widget_button.h"
#include "widget_hslider.h"
#include "widget_state.h"
#include "widget_vslider.h"

struct SequencerUI {
  Sequencer *sequencer = NULL;
  HSlider stepButtons[Sequencer::MAX_STEPS];
  Button playButton;

  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  SequencerUI(Sequencer *_sequencer) : sequencer(_sequencer) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto pageMargin = 50;
    auto width = (shape.halfSize.x * 2);
    auto height = (shape.halfSize.y * 2);
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;
    const int numButtonsPerRow = 4;
    auto buttonWidth = (width - pageMargin) / numButtonsPerRow;
    auto buttonHeight = buttonWidth;
    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {
      stepButtons[i] = HSlider{
          .shape = {
              .position = {.x = static_cast<float>(
                               buttonWidth * (i % numButtonsPerRow) +
                               buttonWidth / 2.0 + xOffset + pageMargin / 2.0),
                           .y = static_cast<float>(
                               buttonHeight * floor(i / numButtonsPerRow) +
                               buttonHeight / 2.0 + yOffset + pageMargin)},
              .halfSize = {.x = static_cast<float>(buttonWidth / 2.0 - 10),
                           .y = static_cast<float>(buttonHeight / 2.0 - 10)}}};
    }
    playButton = Button{
        .label = Label("play"),
        .shape = {.position = {.x = shape.position.x,
                               .y = static_cast<float>(shape.halfSize.y * 2 -
                                                       buttonHeight / 2.0 -
                                                       pageMargin / 2.0)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 4.0)}}};
  };

  void handleFingerMove(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure) {

    if (fingerPositions[fingerId] > -1) {
      DoHSliderDrag(&stepButtons[fingerPositions[fingerId]],
                    &sequencer->stepValues[fingerPositions[fingerId]],
                    position);
    }
  };

  void handleFingerDown(const SDL_FingerID &fingerId, const vec2f_t &position,
                        const float pressure) {

    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {
      if (DoHSliderClick(&stepButtons[i], &sequencer->stepValues[i],
                         position)) {

        fingerPositions[fingerId] = i;
      }
    }
  };

  void handleFingerUp(const SDL_FingerID &fingerId, const vec2f_t &position,
                      const float pressure) {

    auto i = fingerPositions[fingerId];
    if (i > -1) {
      stepButtons[i].state = INACTIVE;
    }
    fingerPositions[fingerId] = -1;
  };

  void handleMouseMove(const vec2f_t &mousePosition){};

  void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoButtonClick(&playButton, mousePosition)) {
      sequencer->running = !sequencer->running;

      if (sequencer->running) {
        playButton.label.setText("stop");
      } else {
        playButton.label.setText("play");
      }
    }
  };

  void handleMouseUp(const vec2f_t &mousePosition){};

  void draw(SDL_Renderer *renderer, const Style &style) {
    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {
      if (i == sequencer->currentStep) {
        stepButtons[i].state = HOVER;
      } else if (stepButtons[i].state != ACTIVE) {
        stepButtons[i].state = INACTIVE;
      }
      DrawHSlider(stepButtons[i], sequencer->stepValues[i], renderer, style);
    }
    DrawButton(&playButton, renderer, style);
  };
};
