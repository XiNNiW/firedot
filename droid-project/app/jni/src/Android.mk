LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -std=gnu++17
LOCAL_MODULE := algae
LOCAL_SRC_FILES := $(LOCAL_PATH)/../algae/src/algae.cpp
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../algae/src/
#LOCAL_EXPORT_C_INCLUDES := /usr/local/include
include $(BUILD_STATIC_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_CFLAGS := -std=gnu++17
#LOCAL_MODULE := SDL_image
#SDL_IMAGE_PATH := ../SDL_image
#LOCAL_SRC_FILES := $(LOCAL_PATH)/$(SDL_IMAGE_PATH)
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(SDL_IMAGE_PATH)/include
#include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
#LOCAL_CFLAGS += -std=gnu++17
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
SDL_AUDIO_DRIVER_AAUDIO := 0
LOCAL_MODULE := main
SDL_PATH := ../SDL2
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../SDL2/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../SDL2_image/include $(LOCAL_PATH)/../SDL2_image
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../SDL2_ttf
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../SPSCQueue/include
$(info $$LOCAL_C_INCLUDES is [${LOCAL_C_INCLUDES}])
# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c ../../../../main.cpp
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_ttf SDL2_image
LOCAL_STATIC_LIBRARIES := algae
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid -ldl

#SUPPORT_HARFBUZZ := false
APP_ALLOW_MISSING_DEPS := true
include $(BUILD_SHARED_LIBRARY)