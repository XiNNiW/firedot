#cmake_minimum_required(VERSION 3.6)
#
#project(MY_APP)
#
#find_library(SDL2 SDL2)
#
#add_library(main SHARED)
#
#target_sources(main PRIVATE YourSourceHere.c)
#
#target_link_libraries(main SDL2)

cmake_minimum_required(VERSION 3.6)

project(PHYS_DEMO_APP)
set(SDL_AUDIO_DRIVER_OPENSLES 1)
set(SDL_AUDIO_DRIVER_AAUDIO 0)


find_library(SDL2 SDL2)
find_library(SDL2_image SDL2_image)
find_library(algae algae)

add_library(main SHARED)

target_sources(main PRIVATE ../../../../main.cpp)

target_link_libraries(main SDL2 SDL2_image algae
        android
        log
        EGL
        GLESv1_CM
        OpenSLES
        GLESv2)

