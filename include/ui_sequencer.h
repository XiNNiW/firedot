#pragma once

#include "SDL_timer.h"
#include "sequencer.h"
#include "vector_math.h"
#include "widget_button.h"
#include "widget_hslider.h"
#include "widget_state.h"
#include "widget_style.h"
#include "widget_vslider.h"
#include <cstdint>
#include <sstream>
struct TempoDetector {
  constexpr static const double MAX_INTERVAL_SECONDS = (1.0 / 24.0) * 60.0;
  float tempo = 0;
  double firstClickTimeSeconds = 0;
  bool DoTapTempo(Button *button, double currentTimeSeconds,
                  const vec2f_t &mousePosition) {
    if (DoButtonClick(button, mousePosition)) {
      auto intervalSeconds = currentTimeSeconds - firstClickTimeSeconds;
      auto intervalMinutes = intervalSeconds / 60.0;
      auto estimate = 1.0 / intervalMinutes;
      if ((abs(tempo - estimate) < 10) && (tempo < 300) && (tempo > 15)) {
        tempo = (tempo + estimate) / 2.0;
        firstClickTimeSeconds = currentTimeSeconds;
        return true;
      } else {
        tempo = estimate;
        firstClickTimeSeconds = currentTimeSeconds;
      }
    }
    return false;
  }
};
struct SequencerUI {
  Sequencer *sequencer = NULL;
  HSlider stepButtons[Sequencer::MAX_STEPS];
  Button playButton;
  HSlider tempoSlider;
  Button tapTempoButton;
  HSlider seqLengthSlider;

  TempoDetector tempoDetector;

  int fingerPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  SequencerUI(Sequencer *_sequencer) : sequencer(_sequencer) {}

  void buildLayout(const AxisAlignedBoundingBox &shape) {
    auto pageMargin = shape.halfSize.y / 32;
    auto width = (shape.halfSize.x * 2);
    auto height = (shape.halfSize.y * 2);
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;
    const int numButtonsPerRow = 4;
    auto sequencerAreaHeight = height * (2.0 / 3.0);
    auto buttonHeight = fmin(sequencerAreaHeight / 4.0,
                             (width - pageMargin) / numButtonsPerRow);
    auto buttonWidth = buttonHeight;
    auto sliderHeight = buttonHeight;
    auto buttonMargin = shape.halfSize.x / 32.0;
    auto editAreaWidth = buttonWidth * 4 - buttonMargin * 2;

    auto yPos = yOffset;
    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {
      auto x = shape.position.x - 1.5 * buttonWidth +
               (i % numButtonsPerRow) * buttonWidth;
      stepButtons[i] = HSlider{
          .shape = {
              .position = {.x = static_cast<float>(x),
                           .y = static_cast<float>(
                               buttonHeight * floor(i / numButtonsPerRow) +
                               buttonHeight / 2.0 + yPos)},
              .halfSize = {
                  .x = static_cast<float>(buttonWidth / 2.0 - buttonMargin),
                  .y = static_cast<float>(buttonHeight / 2.0 - buttonMargin)}}};
    }
    yPos += buttonHeight * floor(Sequencer::MAX_STEPS / numButtonsPerRow);

    auto remainingYSpace = height / 3.0;
    auto elementHalfHeight = (remainingYSpace / 4.0) / 2.0;

    auto tempoButtonHalfWidth = shape.halfSize.x / 5.0;
    auto tempoSliderShape = AxisAlignedBoundingBox{
        .position = {.x = static_cast<float>(shape.position.x -
                                             tempoButtonHalfWidth),
                     .y = static_cast<float>(yPos + elementHalfHeight +
                                             pageMargin - 15)},
        .halfSize = {
            .x = static_cast<float>(editAreaWidth / 2.0 - tempoButtonHalfWidth),
            .y = static_cast<float>(elementHalfHeight)}};
    std::stringstream sstream;
    sstream << "tempo: " << sequencer->getTempo();
    tempoSlider = HSlider{
        .label = Label(tempoSliderShape, sstream.str()),
        .shape = tempoSliderShape,
    };
    auto tapTempoButtonShape = AxisAlignedBoundingBox{
        .position = {.x = static_cast<float>(tempoSlider.shape.position.x +
                                             tempoSlider.shape.halfSize.x +
                                             buttonMargin / 2.0 +
                                             tempoButtonHalfWidth),
                     .y = tempoSlider.shape.position.y},
        .halfSize = {.x = static_cast<float>(tempoButtonHalfWidth),
                     .y = static_cast<float>(elementHalfHeight)}};
    tapTempoButton = Button{.label = Label(tapTempoButtonShape, "tap"),
                            .shape = tapTempoButtonShape};
    auto seqLengthShape = AxisAlignedBoundingBox{
        .position = {.x = static_cast<float>(shape.position.x),
                     .y = static_cast<float>(tempoSliderShape.position.y +
                                             elementHalfHeight * 2 +
                                             pageMargin / 4.0)},
        .halfSize = {.x = static_cast<float>(editAreaWidth / 2.0),
                     .y = static_cast<float>(elementHalfHeight)}};
    seqLengthSlider = HSlider{.label = Label(seqLengthShape, "length"),
                              .shape = seqLengthShape};
    auto playButtonShape = AxisAlignedBoundingBox{
        .position = {.x = shape.position.x,
                     .y = static_cast<float>(
                         shape.position.y + shape.halfSize.y -
                         buttonHeight / 2.0 - pageMargin / 2.0)},
        .halfSize = {.x = static_cast<float>(elementHalfHeight),
                     .y = static_cast<float>(elementHalfHeight)}};
    playButton = Button{
        .label = Label(playButtonShape, "play"),
        .shape = playButtonShape,
        .iconType = sequencer->isRunning() ? IconType::STOP : IconType::PLAY,
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
      std::stringstream sstream;
      sstream << "tempo: " << tempoDetector.tempo;
      tempoSlider.label.setText(sstream.str());
    };
    float lengthSliderValue = sequencer->getLengthNormalized();
    if (DoHSliderDrag(&seqLengthSlider, &lengthSliderValue, mousePosition)) {
      sequencer->setLength(lengthSliderValue * sequencer->MAX_STEPS);
    };
  };

  void handleMouseDown(const vec2f_t &mousePosition) {
    if (DoButtonClick(&playButton, mousePosition)) {
      // sequencer->running = !sequencer->running;
      sequencer->toggleRunning();
      if (sequencer->isRunning()) {
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
      std::stringstream sstream;
      sstream << "tempo: " << sequencer->getTempo();
      tempoSlider.label.setText(sstream.str());
    };

    if (tempoDetector.DoTapTempo(&tapTempoButton, SDL_GetTicks() / 1000.0,
                                 mousePosition)) {
      sequencer->setTempo(tempoDetector.tempo);
      std::stringstream sstream;
      sstream << "tempo: " << sequencer->getTempo();
      tempoSlider.label.setText(sstream.str());
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
    DrawButton(&playButton, renderer, style, style.color2, style.color2);
    DrawButton(&tapTempoButton, renderer, style);
    auto tempoRange = (sequencer->maxBPM - sequencer->minBPM);
    DrawHSlider(&seqLengthSlider, sequencer->getLengthNormalized(), renderer,
                style);
    DrawHSlider(&tempoSlider, sequencer->getTempoNormalized(), renderer, style);
  };
};
