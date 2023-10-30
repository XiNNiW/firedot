cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
project(KEY_SDL_DEMO)

# armeabi-v7a requires cpufeatures library
# include(AndroidNdkModules)
# android_ndk_import_module_cpufeatures()

#set(FREETYPE_DIR SDL2_ttf/external/freetype)
# SDL sources are in a subfolder named "SDL"
add_subdirectory(SDL2)

# Compilation of companion libraries
add_subdirectory(SDL2_image)
add_subdirectory(SDL2_ttf)
add_subdirectory(algae)
add_subdirectory(SPSCQueue)


add_compile_definitions(SDL_AUDIODRIVER="openslES")
# Your game and its CMakeLists.txt are in a subfolder named "src"
add_subdirectory(src)
