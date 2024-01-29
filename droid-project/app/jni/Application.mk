
# Uncomment this if you're using STL in your project
# You can find more information here:
# https://developer.android.com/ndk/guides/cpp-support
# APP_STL := c++_shared
APP_CPPFLAGS += -std=c++17

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
# APP_STL := system
# Min runtime API level
APP_PLATFORM=android-16

SDL_AUDIODRIVER="openslES"

LOCAL_CPPFLAGS += -fexceptions
LOCAL_CPPFLAGS += -frtti

#SDL2_image

