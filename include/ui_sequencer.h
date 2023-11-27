#pragma once

#include "sequencer.h"
#include "ui_abstract.h"
#include "widget_vslider.h"
struct SequencerUI : public AbstractUI {
  Sequencer *sequencer = NULL;
  VSlider stepButtons[Sequencer::MAX_STEPS];

  SequencerUI(Sequencer *_sequencer) : sequencer(_sequencer) {}

  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    const int numButtonsPerRow = 4;
    auto buttonWidth = shape.halfSize.x * 2 / numButtonsPerRow;
    auto buttonHeight = buttonWidth;
    for (int i = 0; i < Sequencer::MAX_STEPS; i++) {
      stepButtons[i] = VSlider{
          .shape = {.position = {.x = buttonWidth * i + buttonWidth / 2,
                                 .y = buttonHeight * int(i / numButtonsPerRow) +
                                      buttonHeight / 2},
                    .halfSize = {.x = buttonWidth / 2, .y = buttonHeight / 2}}};
    }
  };

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure) {
    for (auto &slider : stepButtons) {
      DoVSliderDrag(&slider, position);
    }
  };

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure){

  };

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position, const float pressure){};

  virtual void handleMouseMove(const vec2f_t &mousePosition){};

  virtual void handleMouseDown(const vec2f_t &mousePosition){};

  virtual void handleMouseUp(const vec2f_t &mousePosition){};

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
    int index = 0;
    for (auto &slider : stepButtons) {
      if (index == sequencer->currentStep) {
        slider.state = ACTIVE;
      }
      DrawVSlider(slider, renderer, style);
      ++index;
    }
  };
};
