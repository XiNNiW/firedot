#pragma once

#include "metaphor.h"
#include "ui_abstract.h"
#include "ui_keyboard.h"
#include "ui_sequencer.h"
struct PlayInstrumentUI : AbstractUI {
  InstrumentMetaphorType *instrumentMetaphor;
  KeyboardUI keyboardUI;
  SequencerUI sequencerUI;

  PlayInstrumentUI(Synthesizer<float> *synth, Sequencer *sequencer)
      : keyboardUI(KeyboardUI(synth)), sequencerUI(SequencerUI(sequencer)) {}

  virtual void buildLayout(const AxisAlignedBoundingBox &shape) {
    keyboardUI.buildLayout(shape);
    sequencerUI.buildLayout(shape);
  };

  virtual void handleFingerMove(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.handleFingerMove(fingerId, position, pressure);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.handleFingerMove(fingerId, position, pressure);
      break;
    }
  };

  virtual void handleFingerDown(const SDL_FingerID &fingerId,
                                const vec2f_t &position, const float pressure) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.handleFingerDown(fingerId, position, pressure);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.handleFingerDown(fingerId, position, pressure);
      break;
    }
  };

  virtual void handleFingerUp(const SDL_FingerID &fingerId,
                              const vec2f_t &position, const float pressure) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.handleFingerUp(fingerId, position, pressure);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.handleFingerUp(fingerId, position, pressure);
      break;
    }
  };

  virtual void handleMouseMove(const vec2f_t &mousePosition) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.handleMouseMove(mousePosition);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.handleMouseMove(mousePosition);
      break;
    }
  };

  virtual void handleMouseDown(const vec2f_t &mousePosition) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.handleMouseDown(mousePosition);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.handleMouseDown(mousePosition);
      break;
    }
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.handleMouseUp(mousePosition);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.handleMouseUp(mousePosition);
      break;
    }
  };

  virtual void draw(SDL_Renderer *renderer, const Style &style) {
    switch (*instrumentMetaphor) {
    case InstrumentMetaphorType::KEYBOARD:
      keyboardUI.draw(renderer, style);
      break;
    case InstrumentMetaphorType::SEQUENCER:
      sequencerUI.draw(renderer, style);
      break;
    }
  };
};
