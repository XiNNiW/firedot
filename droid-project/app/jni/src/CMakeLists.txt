cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
project(KEY_SDL_DEMO)

find_library(SDL2 SDL2)
find_library(SDL2_image SDL2_image)
find_library(SDL2_ttf SDL2_ttf)
find_library(algae algae)
find_library(SPSCQueue SPSCQueue)
add_library(main SHARED)
#add_library(algae STATIC IMPORTED)
#set_target_properties(algae PROPERTIES IMPORTED_LOCATION algae/bin/libalgae.a)
#set_target_properties(algae PROPERTIES INCLUDE_DIRECTORIES algae/src)


file(GLOB SOURCES "../../../../src/*.cpp")

target_sources(main PRIVATE ../../../../main.cpp ${SOURCES})


target_link_libraries(main SDL2 SDL2_image algae
        SDL2_ttf
        android
        log
        EGL
        GLESv1_CM
        OpenSLES
        GLESv2
        SPSCQueue
        )


