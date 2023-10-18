cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
project(PHYS_DEMO_APP)
# set(SDL_AUDIO_DRIVER_OPENSLES 1)
# set(SDL_AUDIO_DRIVER_AAUDIO 0)

#set(FREETYPE_DIR SDL2_ttf/external/freetype)

find_library(SDL2 SDL2)
find_library(SDL2_image SDL2_image)
#find_library(freetype freetype)
#find_library(SDL2_ttf SDL2_ttf)
find_library(algae algae)

add_library(main SHARED)

target_sources(main PRIVATE ../../../../main.cpp)

target_link_libraries(main SDL2 SDL2_image algae
#        SDL2_ttf
        android
        log
        EGL
        GLESv1_CM
        OpenSLES
        GLESv2
#        freetype
        )


