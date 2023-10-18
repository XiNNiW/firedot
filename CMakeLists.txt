cmake_minimum_required (VERSION 3.7)
include(CMakePrintHelpers)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
# Below line is for using C++14 standard
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Our project name
project (imu_synth)

add_executable(imu_synth main.cpp)

# For finding our SDL2 package!
INCLUDE(FindPkgConfig)

#set(SDL_AUDIO_DRIVER_AAUDIO 0)
#set(SDL_AUDIO_DRIVER_OPENSLES 1)

# Searching SDL2 package.
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)


cmake_print_variables(SDL2_IMAGE_INCLUDE_DIRS)
cmake_print_variables(SDL2TTF_INCLUDE_DIR)
cmake_print_variables(SDL2_IMAGE_LIBRARIES)
cmake_print_variables(SDL2TTF_LIBRARY)

add_compile_definitions(SDL_AUDIODRIVER="jack")

set(ALGAE_INCLUDE_DIRS, "/usr/local/include/")
set(ALGAE_LIBRARIES, "/usr/local/lib/libalgae.a")
# Including our external library's directory for header files!
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIR} ${ALGAE_INCLUDE_DIRS})

# Linking SDL2 library with our project.
target_link_libraries(imu_synth ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2TTF_LIBRARY} ${ALGAE_LIBRARIES})
