#pragma once
#include "SDL_render.h"
#include "SDL_video.h"
#include "collider.h"
#include "ui_navigation.h"
#include "widget_button.h"
#include "widget_state.h"
#include "widget_utils.h"
#include <dirent.h>
#include <sstream>
#include <string>
#include <vector>

struct FilebrowserUI {
  enum Mode { CLOSED, OPEN, SELECTED } mode = CLOSED;
  std::vector<Button> fileButtons;
  Button selectButton;
  Button cancelButton;
  std::string selectedPath = "";
  Label title = Label("choose a file");
  AxisAlignedBoundingBox shape;

  FilebrowserUI() {}

  inline void open() { mode = OPEN; }

  inline void close() { mode = CLOSED; }

  void buildLayout(AxisAlignedBoundingBox _shape) {
    selectedPath = "";
    shape = {.position = _shape.position,
             .halfSize = _shape.halfSize.scale(0.85)};
    auto xOffset = shape.position.x - shape.halfSize.x;
    auto yOffset = shape.position.y - shape.halfSize.y;
    auto width = shape.halfSize.x * 2;
    auto height = shape.halfSize.y * 2;

    title.shape = {
        .position = {.x = shape.position.x, .y = yOffset + height / 50},
        .halfSize = {.x = shape.halfSize.x, .y = height / 50}};

    // const path saveDirectory = path{SDL_GetBasePath()} / "save_files";
    // if (!exists(saveDirectory)) {
    //   create_directory(saveDirectory);
    // }
    // const auto numFiles =
    //     std::distance(boost::filesystem::directory_iterator(saveDirectory),
    //                   boost::filesystem::directory_iterator{});

    //  struct dirent *entry = nullptr;
    //  DIR *dp = nullptr;

    //  std::stringstream savefilepathstream;
    //  savefilepathstream << SDL_GetBasePath() << "save_files";
    //  dp = opendir(savefilepathstream.str().c_str());
    //  int numFiles = 0;
    //  if (dp != nullptr) {
    //    while ((entry = readdir(dp))) {
    //      if (entry->d_type == DT_DIR)
    //        continue;
    //      ++numFiles;
    //    }
    //  }

    //  closedir(dp);

    //  auto buttonHeight =
    //      numFiles > 0
    //          ? std::min(shape.halfSize.y * 2.0 /
    //          static_cast<float>(numFiles),
    //                     50.0)
    //          : 50;
    //  int fileCounter = 0;
    // for (auto const &dir_entry :
    //      boost::filesystem::directory_iterator{saveDirectory}) {
    //   fileButtons.push_back(Button{
    //       .label = Label(dir_entry.path().filename().string()),
    //       .shape = {.position = {.x = shape.position.x,
    //                              .y = static_cast<float>(
    //                                  yOffset + buttonHeight *
    //                                  fileCounter++)},
    //                 .halfSize = {.x = shape.halfSize.x,
    //                              .y = static_cast<float>(buttonHeight / 2)}},
    //   });
    // }

    //   dp = opendir(savefilepathstream.str().c_str());
    //   if (dp != nullptr) {
    //     while ((entry = readdir(dp))) {
    //       if (entry->d_type == DT_DIR)
    //         continue;

    //       fileButtons.push_back(Button{
    //           .label = Label(entry->d_name),
    //           .shape = {.position = {.x = shape.position.x,
    //                                  .y = static_cast<float>(
    //                                      yOffset + buttonHeight *
    //                                      fileCounter++)},
    //                     .halfSize = {.x = shape.halfSize.x,
    //                                  .y = static_cast<float>(buttonHeight /
    //                                  2)}},
    //       });
    //     }
    //   }

    //   closedir(dp);

    auto buttonMargin = shape.halfSize.x / 24;
    auto buttonHalfWidth = shape.halfSize.x / 2 - buttonMargin;
    auto buttonHalfHeight = shape.halfSize.y / 8 - buttonMargin;
    cancelButton =
        Button{.label = Label("cancel"),
               .shape = {.position = {.x = xOffset + shape.halfSize.x -
                                           buttonHalfWidth - buttonMargin,
                                      .y = yOffset + height - buttonHalfHeight -
                                           buttonMargin},
                         .halfSize = {buttonHalfWidth, buttonHalfHeight}}};
    selectButton =
        Button{.label = Label("select"),
               .shape = {.position = {.x = xOffset + shape.halfSize.x +
                                           buttonHalfWidth + buttonMargin,
                                      .y = yOffset + height - buttonHalfHeight -
                                           buttonMargin},
                         .halfSize = {buttonHalfWidth, buttonHalfHeight}}};
  }

  virtual void handleMouseMove(const vec2f_t &mousePosition){};

  virtual void handleMouseDown(const vec2f_t &mousePosition) {
    for (auto &button : fileButtons) {
      if (DoButtonClick(&button, mousePosition)) {
        selectedPath = button.label.getText();
      }
    }
    if (DoButtonClick(&cancelButton, mousePosition)) {
      close();
    }
    if (DoButtonClick(&selectButton, mousePosition)) {
      mode = SELECTED;
    }
  };

  virtual void handleMouseUp(const vec2f_t &mousePosition){};

  void draw(SDL_Renderer *renderer, const Style &style) {
    if (mode == OPEN) {
      // int width = 0, height = 0;
      // SDL_GetRendererOutputSize(renderer, &width, &height);
      // AxisAlignedBoundingBox screenRect = AxisAlignedBoundingBox{
      //     .position = {.x = static_cast<float>(width / 2.0),
      //                  .y = static_cast<float>(height / 2.0)},
      //     .halfSize = {.x = static_cast<float>(width / 2.0),
      //                  .y = static_cast<float>(height / 2.0)}};
      DrawFilledRect(shape, renderer, SDL_Color{0, 0, 0, 0});
      DrawFilledRect(title.shape, renderer, style.color1);
      title.draw(style.inactiveColor, style.color1, renderer, style,
                 HorizontalAlignment::CENTER, VerticalAlignment::CENTER);
      DrawBoxOutline(shape, renderer, style.color1);
      for (auto &button : fileButtons) {
        DrawButton(&button, renderer, style);
      }
      DrawButton(&cancelButton, renderer, style);
      DrawButton(&selectButton, renderer, style);
    }
  }
};
