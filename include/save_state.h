#pragma once

#include "mapping.h"
#include "metaphor.h"
#include "pitch_collection.h"
#include "synthesis.h"
#include "synthesis_parameter.h"
#include "synthesizer_settings.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class SaveState {
private:
  InstrumentMetaphorType instrumentMetaphor = KEYBOARD;
  SynthesizerSettings synthesizerSettings;

public:
  InputMapping<float> sensorMapping;

  inline InstrumentMetaphorType getInstrumentMetaphorType() const {
    return instrumentMetaphor;
  }
  inline const SynthesizerSettings &getSynthesizerSettings() const {
    return synthesizerSettings;
  }
  inline void setInstrumentMetaphor(InstrumentMetaphorType type) {
    instrumentMetaphor = type;
  }

  inline static bool SaveGame(const std::string &filename,
                              const Synthesizer<float> &synth,
                              SaveState *state) {
    state->synthesizerSettings.synthType = synth.getSynthType();
    state->synthesizerSettings.gain = synth.getParameter(GAIN);
    state->synthesizerSettings.soundSource = synth.getParameter(SOUND_SOURCE);
    state->synthesizerSettings.filterCutoff = synth.getParameter(FILTER_CUTOFF);
    state->synthesizerSettings.filterQuality =
        synth.getParameter(FILTER_QUALITY);
    state->synthesizerSettings.attack = synth.getParameter(ATTACK_TIME);
    state->synthesizerSettings.release = synth.getParameter(RELEASE_TIME);
    SDL_Log("save state!");
    std::ofstream save("game_save");
    save << "[instrumentMetaphor]"
         << "\n";
    save << std::to_string(state->instrumentMetaphor) << "\n\n";
    save << "[momentaryMappings]"
         << "\n";
    for (auto &modePair : state->sensorMapping.instrumentModeSpecificMappings) {
      for (auto &paramInputPair : modePair.second.momentaryMappings) {
        save << std::to_string(modePair.first) << ","
             << std::to_string(paramInputPair.first) << ","
             << std::to_string(paramInputPair.second) << "\n";
      }
    }

    save << "\n";
    save << "[continuousMappings]"
         << "\n";
    for (auto &modePair : state->sensorMapping.instrumentModeSpecificMappings) {
      for (auto &pair : modePair.second.continuousMappings) {
        save << std::to_string(modePair.first) << ","
             << std::to_string(pair.first) << "," << std::to_string(pair.second)
             << "\n";
      }
    }
    save << "\n";
    save << "[soundSettings]"
         << "\n";
    save << state->synthesizerSettings.synthType << ",";
    save << state->synthesizerSettings.gain << ",";
    save << state->synthesizerSettings.soundSource << ",";
    save << state->synthesizerSettings.filterCutoff << ",";
    save << state->synthesizerSettings.filterQuality << ",";
    save << state->synthesizerSettings.attack << ",";
    save << state->synthesizerSettings.release << ",";
    save << state->synthesizerSettings.octave << "\n\n";
    save << "[scaleType]"
         << "\n";
    save << static_cast<int>(state->sensorMapping.key) << ","
         << static_cast<int>(state->sensorMapping.scaleType) << "\n";

    save << "\n";
    save.close();
    auto failed = save.fail();
    if (failed) {
      SDL_LogError(0, "save failed!");
    }
    return !failed;
  }

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

            if (seglist.size() == 3) {
              state->sensorMapping.addMapping(
                  static_cast<InstrumentMetaphorType>(std::stoi(seglist[0])),
                  static_cast<MomentaryInputType>(std::stoi(seglist[2])),
                  static_cast<MomentaryParameterType>(std::stoi(seglist[1])));
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

            if (seglist.size() == 3) {
              state->sensorMapping.addMapping(
                  static_cast<InstrumentMetaphorType>(std::stoi(seglist[0])),
                  static_cast<ContinuousInputType>(std::stoi(seglist[2])),
                  static_cast<ContinuousParameterType>(std::stoi(seglist[1])));
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

          std::stringstream lineStream(lineText);
          std::string segment;
          std::vector<std::string> seglist;

          while (std::getline(lineStream, segment, ',')) {
            seglist.push_back(segment);
          }
          if (seglist.size() == 2) {
            state->sensorMapping.key = std::stoi(seglist[0]);
            state->sensorMapping.scaleType =
                static_cast<ScaleType>(std::stoi(seglist[1]));
          }
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
};
