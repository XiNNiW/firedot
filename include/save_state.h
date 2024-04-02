#pragma once

#include "SDL_audio.h"
#include "SDL_filesystem.h"
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

  inline static void
  copySynthSettingsToCurrentMapping(SaveState *saveState,
                                    const Synthesizer<float> &synth) {

    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.synthType = synth.getSynthType();
    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.gain = synth.getParameter(GAIN);
    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.soundSource = synth.getParameter(SOUND_SOURCE);
    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.filterCutoff = synth.getParameter(FILTER_CUTOFF);
    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.filterQuality = synth.getParameter(FILTER_QUALITY);
    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.attack = synth.getParameter(ATTACK_TIME);
    saveState->sensorMapping
        .instrumentModeSpecificMappings[saveState->instrumentMetaphor]
        .synthesizerSettings.release = synth.getParameter(RELEASE_TIME);
  }

  inline void
  copyCurrentMappingSettingsToSynthesizer(Synthesizer<float> *synth) {

    synth->setSynthType(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.synthType);
    synth->setGain(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.gain);
    synth->setSoundSource(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.soundSource);
    synth->setAttackTime(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.attack);
    synth->setReleaseTime(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.release);
    synth->setFilterCutoff(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.filterCutoff);
    synth->setFilterQuality(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.filterQuality);
    synth->setOctave(
        sensorMapping.instrumentModeSpecificMappings[instrumentMetaphor]
            .synthesizerSettings.octave);
  }

public:
  InputMapping<float> sensorMapping;

  inline InstrumentMetaphorType getInstrumentMetaphorType() const {
    return instrumentMetaphor;
  }
  inline const SynthesizerSettings &getSynthesizerSettings() const {
    return synthesizerSettings;
  }
  inline void setInstrumentMetaphor(InstrumentMetaphorType type,
                                    Synthesizer<float> *synth) {
    copySynthSettingsToCurrentMapping(this, *synth);
    instrumentMetaphor = type;
    copyCurrentMappingSettingsToSynthesizer(synth);
  }

  inline static bool SaveGame(const std::string &filename,
                              const Synthesizer<float> &synth,
                              SaveState *state) {

    copySynthSettingsToCurrentMapping(state, synth);
    SDL_Log("save state!");
    std::stringstream saveFilePath;
    saveFilePath << SDL_GetPrefPath("lichensound", "firedot");
    saveFilePath << "/game_save";
    std::ofstream save(saveFilePath.str());
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
    for (auto &modePair : state->sensorMapping.instrumentModeSpecificMappings) {

      save << std::to_string(modePair.first) << ",";
      save << modePair.second.synthesizerSettings.synthType << ",";
      save << modePair.second.synthesizerSettings.gain << ",";
      save << modePair.second.synthesizerSettings.soundSource << ",";
      save << modePair.second.synthesizerSettings.filterCutoff << ",";
      save << modePair.second.synthesizerSettings.filterQuality << ",";
      save << modePair.second.synthesizerSettings.attack << ",";
      save << modePair.second.synthesizerSettings.release << ",";
      save << modePair.second.synthesizerSettings.octave << "\n";
    }
    save << "\n";

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
    std::stringstream saveFilePath;
    saveFilePath << SDL_GetPrefPath("lichensound", "firedot");
    saveFilePath << "/game_save";
    std::ifstream load(saveFilePath.str());
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
        if (headingMap.count(lineText) == 0) {
          SDL_Log("map did not contain %s", lineText.c_str());
          break;
        }
        switch (headingMap[lineText]) {
        case FileHeading::INSTRUMENT_METAPHOR: {
          fileHeading = FileHeading::INSTRUMENT_METAPHOR;
          readState = ReadState::READING;
          break;
        }
        case FileHeading::MOMENTARY_MAPPING: {
          fileHeading = FileHeading::MOMENTARY_MAPPING;
          readState = ReadState::READING;
          break;
        }
        case FileHeading::CONTINUOUS_MAPPING: {
          fileHeading = FileHeading::CONTINUOUS_MAPPING;
          readState = ReadState::READING;
          break;
        }
        case FileHeading::SYNTH_SETTINGS: {
          fileHeading = FileHeading::SYNTH_SETTINGS;
          readState = ReadState::READING;
          break;
        }
        case FileHeading::SCALE_TYPE: {
          fileHeading = FileHeading::SCALE_TYPE;
          readState = ReadState::READING;
          break;
        }
        default: {
          SDL_Log("%s", lineText.c_str());
          readState = ReadState::SEARCHING;
          break;
        }
        }

        break;
      }
      case ReadState::READING: {
        switch (fileHeading) {

        case FileHeading::INSTRUMENT_METAPHOR: {
          SDL_Log("loading instrument metaphor");
          state->instrumentMetaphor =
              static_cast<InstrumentMetaphorType>(std::stoi(lineText));
          readState = ReadState::SEARCHING;
          break;
        }
        case FileHeading::MOMENTARY_MAPPING: {

          SDL_Log("loading momentaryMappings");
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
              InstrumentMetaphorType mode =
                  static_cast<InstrumentMetaphorType>(std::stoi(seglist[0]));
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.synthType =
                  static_cast<SynthesizerType>(std::stoi(seglist[1]));
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.gain = std::stof(seglist[2]);
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.soundSource = std::stof(seglist[3]);
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.filterCutoff = std::stof(seglist[4]);
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.filterQuality = std::stof(seglist[5]);
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.attack = std::stof(seglist[6]);
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.release = std::stof(seglist[7]);
              state->sensorMapping.instrumentModeSpecificMappings[mode]
                  .synthesizerSettings.octave = std::stof(seglist[8]);
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
