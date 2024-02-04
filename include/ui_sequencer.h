#pragma once

#include "sequencer.h"
#include "widget_button.h"
#include "widget_hslider.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_vslider.h"

struct SequencerUI {
  Sequencer *sequencer = NULL;
  HSlider stepButtons[Sequencer::MAX_STEPS];
  Button playButton;
  HSlider tempoSlider;
  HSlider seqLengthSlider;

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
    auto sliderHeight = buttonHeight;
    auto yPos = yOffset + pageMargin;
    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {
      stepButtons[i] = HSlider{
          .shape = {
              .position = {.x = static_cast<float>(
                               buttonWidth * (i % numButtonsPerRow) +
                               buttonWidth / 2.0 + xOffset + pageMargin / 2.0),
                           .y = static_cast<float>(
                               buttonHeight * floor(i / numButtonsPerRow) +
                               buttonHeight / 2.0 + yPos)},
              .halfSize = {.x = static_cast<float>(buttonWidth / 2.0 - 10),
                           .y = static_cast<float>(buttonHeight / 2.0 - 10)}}};
    }
    yPos += buttonHeight * 4 + buttonHeight / 2.0;

    playButton = Button{
        .label = Label("play"),
        .shape = {.position = {.x = shape.position.x,
                               .y = static_cast<float>(
                                   shape.position.y + shape.halfSize.y -
                                   buttonHeight / 2.0 - pageMargin / 2.0)},
                  .halfSize = {.x = static_cast<float>(buttonWidth / 2.0),
                               .y = static_cast<float>(buttonHeight / 4.0)}},
        .iconType = sequencer->running ? IconType::STOP : IconType::PLAY,
    };

    tempoSlider = HSlider{
        .label = Label("tempo"),
        .shape = {.position = {.x = shape.position.x,
                               .y = static_cast<float>(shape.position.y +
                                                       shape.halfSize.y -
                                                       2 * buttonHeight / 2.0 -
                                                       pageMargin / 2.0 - 15)},
                  .halfSize = {.x = static_cast<float>(shape.halfSize.x -
                                                       pageMargin / 2.0),
                               .y = static_cast<float>(buttonHeight / 6.0)}},
    };

    seqLengthSlider = HSlider{

        .label = Label("length"),
        .shape = {.position = {.x = shape.position.x,
                               .y = static_cast<float>(shape.position.y +
                                                       shape.halfSize.y -
                                                       3 * buttonHeight / 2.0 -
                                                       pageMargin / 2.0 - 15)},
                  .halfSize = {.x = static_cast<float>(shape.halfSize.x -
                                                       pageMargin / 2.0),
                               .y = static_cast<float>(buttonHeight / 6.0)}},
    };
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

  void handleMouseMove(const vec2f_t &mousePosition) {
    auto tempoRange = (sequencer->maxBPM - sequencer->minBPM);
    float sliderValue = sequencer->getTempoNormalized();
    if (DoHSliderDrag(&tempoSlider, &sliderValue, mousePosition)) {
      sequencer->setTempoNormalized(sliderValue);
    };
    float lengthSliderValue = sequencer->getLengthNormalized();
    if (DoHSliderDrag(&seqLengthSlider, &lengthSliderValue, mousePosition)) {
      sequencer->setLength(lengthSliderValue * sequencer->MAX_STEPS);
    };
  };

  void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoButtonClick(&playButton, mousePosition)) {
      sequencer->running = !sequencer->running;

      if (sequencer->running) {
        playButton.label.setText("stop");
        playButton.iconType = IconType::STOP;
      } else {
        playButton.label.setText("play");
        playButton.iconType = IconType::PLAY;
      }
    }
    float tempoSliderValue = sequencer->getTempoNormalized();
    if (DoHSliderClick(&tempoSlider, &tempoSliderValue, mousePosition)) {
      sequencer->setTempoNormalized(tempoSliderValue);
    };
    float lengthSliderValue = sequencer->getLengthNormalized();
    if (DoHSliderClick(&seqLengthSlider, &lengthSliderValue, mousePosition)) {
      sequencer->setLengthNormalized(lengthSliderValue);
    };
  };

  void handleMouseUp(const vec2f_t &mousePosition) {
    tempoSlider.state = INACTIVE;
    seqLengthSlider.state = INACTIVE;
  };

  void draw(SDL_Renderer *renderer, const Style &style) {
    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {

      if (i >= sequencer->getLength())
        break;
      DrawHSlider(&stepButtons[i], sequencer->stepValues[i], renderer, style);
      if (i == sequencer->currentStep) {
        DrawBoxOutline(stepButtons[i].shape, renderer, style.hoverColor);
      }
    }
    DrawButton(&playButton, renderer, style);
    auto tempoRange = (sequencer->maxBPM - sequencer->minBPM);
    DrawHSlider(&seqLengthSlider, sequencer->getLengthNormalized(), renderer,
                style);
    DrawHSlider(&tempoSlider, sequencer->getTempoNormalized(), renderer, style);
  };
};
