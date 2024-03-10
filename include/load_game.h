#pragma once

#include "save_state.h"
#include "synthesis.h"
#include <string>
inline static bool LoadGame(const std::string &filename,
                            Synthesizer<float> *synth, SaveState *state) {
  SDL_Log("load state!");

  std::ifstream load("game_save");
  enum class ReadState {
    SEARCHING,
    READING,
  } readState = ReadState::SEARCHING;
  enum class FileHeading {
    INSTRUMENT_METAPHOR,
    MOMENTARY_MAPPING,
    CONTINUOUS_MAPPING,
    SYNTH_SETTINGS,
    SCALE_TYPE
  } fileHeading;
  std::map<std::string, FileHeading> headingMap;
  headingMap["[instrumentMetaphor]"] = FileHeading::INSTRUMENT_METAPHOR;
  headingMap["[momentaryMappings]"] = FileHeading::MOMENTARY_MAPPING;
  headingMap["[continuousMappings]"] = FileHeading::CONTINUOUS_MAPPING;
  headingMap["[soundSettings]"] = FileHeading::SYNTH_SETTINGS;
  headingMap["[scaleType]"] = FileHeading::SCALE_TYPE;

  std::string lineText;
  while (getline(load, lineText)) {
    SDL_Log("%s", lineText.c_str());
    switch (readState) {

    case ReadState::SEARCHING: {

      switch (headingMap[lineText]) {
      case FileHeading::INSTRUMENT_METAPHOR:
        fileHeading = FileHeading::INSTRUMENT_METAPHOR;
        readState = ReadState::READING;
        break;
      case FileHeading::MOMENTARY_MAPPING:
        fileHeading = FileHeading::MOMENTARY_MAPPING;
        readState = ReadState::READING;
      case FileHeading::CONTINUOUS_MAPPING:
        fileHeading = FileHeading::CONTINUOUS_MAPPING;
        readState = ReadState::READING;
      case FileHeading::SYNTH_SETTINGS:
        fileHeading = FileHeading::SYNTH_SETTINGS;
        readState = ReadState::READING;
        break;
      case FileHeading::SCALE_TYPE:
        fileHeading = FileHeading::SCALE_TYPE;
        readState = ReadState::READING;
        break;
      }

      break;
    }
    case ReadState::READING: {
      switch (fileHeading) {

      case FileHeading::INSTRUMENT_METAPHOR: {

        state->instrumentMetaphor =
            static_cast<InstrumentMetaphorType>(std::stoi(lineText));
        readState = ReadState::SEARCHING;
        break;
      }
      case FileHeading::MOMENTARY_MAPPING: {
        if (lineText.size() == 0) {
          readState = ReadState::SEARCHING;
        } else {
          std::stringstream lineStream(lineText);
          std::string segment;
          std::vector<std::string> seglist;

          while (std::getline(lineStream, segment, ',')) {
            seglist.push_back(segment);
          }

          if (seglist.size() == 2) {
            state->sensorMapping
                .momentaryMappings[static_cast<MomentaryParameterType>(
                    std::stoi(seglist[0]))] =
                static_cast<MomentaryInputType>(std::stoi(seglist[1]));
          }
        }
        break;
      }
      case FileHeading::CONTINUOUS_MAPPING: {
        if (lineText.size() == 0) {
          readState = ReadState::SEARCHING;
        } else {
          std::stringstream lineStream(lineText);
          std::string segment;
          std::vector<std::string> seglist;

          while (std::getline(lineStream, segment, ',')) {
            seglist.push_back(segment);
          }

          if (seglist.size() == 2) {
            state->sensorMapping
                .continuousMappings[static_cast<ContinuousParameterType>(
                    std::stoi(seglist[0]))] =
                static_cast<ContinuousInputType>(std::stoi(seglist[1]));
          }
        }

        break;
      }
      case FileHeading::SYNTH_SETTINGS: {
        if (lineText.size() == 0) {
          readState = ReadState::SEARCHING;
        } else {
          std::stringstream lineStream(lineText);
          std::string segment;
          std::vector<std::string> seglist;

          while (std::getline(lineStream, segment, ',')) {
            seglist.push_back(segment);
          }

          if (seglist.size() == 8) {
            state->synthesizerSettings.synthType =
                static_cast<SynthesizerType>(std::stoi(seglist[0]));
            state->synthesizerSettings.gain = std::stof(seglist[1]);
            state->synthesizerSettings.soundSource = std::stof(seglist[2]);
            state->synthesizerSettings.filterCutoff = std::stof(seglist[3]);
            state->synthesizerSettings.filterQuality = std::stof(seglist[4]);
            state->synthesizerSettings.attack = std::stof(seglist[5]);
            state->synthesizerSettings.release = std::stof(seglist[6]);
            state->synthesizerSettings.octave = std::stof(seglist[7]);
          }
        }

        break;
      }
      case FileHeading::SCALE_TYPE: {
        state->sensorMapping.scaleType =
            static_cast<ScaleType>(std::stoi(lineText));
        readState = ReadState::SEARCHING;

        break;
      }
      }
      break;
    }
    }
  }
  load.close();
  return true;
}
