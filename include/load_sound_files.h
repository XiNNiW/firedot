#pragma once

#ifdef PLATFORM_IS_ANDROID

#include "sample_load.h"
#include "synthesis_sampling.h"
#include <SDL.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <sstream>
#include <string>
inline void LoadSoundFiles(SampleBank<float> *bank) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  jclass clazz(env->GetObjectClass(activity));

  jmethodID getContext_methodID = env->GetStaticMethodID(
      clazz, "getContext", "()Landroid/content/Context;");
  jobject aContext = env->CallStaticObjectMethod(clazz, getContext_methodID);
  jclass aSdlClass = env->FindClass("android/content/Context");
  SDL_Log("%s(): Got Class (%x)", "FUNCTION", aSdlClass);

  jmethodID aJavaMethodID = env->GetMethodID(
      aSdlClass, "getAssets", "()Landroid/content/res/AssetManager;");
  SDL_Log("%s(): Got Method ID (%x)", "FUNCTION", aJavaMethodID);
  jobject aJavaAssetManager = env->CallObjectMethod(aContext, aJavaMethodID);
  AAssetManager *mgr = AAssetManager_fromJava(env, aJavaAssetManager);
  SDL_Log("got manager %x", mgr);
  AAssetDir *assetDir = AAssetManager_openDir(mgr, "sounds");
  SDL_Log("got sounds dur %x", assetDir);
  const char *filename = (const char *)NULL;
  while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
      SDL_Log("about to load %s", filename);
      std::stringstream ss;
      ss << "sounds/" << filename;
    bank->loadSample(ss.str());
  }
  SDL_Log("bank has %d samples", bank->size);
  AAssetDir_close(assetDir);
  env->DeleteLocalRef(activity);
  env->DeleteLocalRef(clazz);
}
#else
#include "sample_load.h"
#include "synthesis_sampling.h"
#include <filesystem>
inline void LoadSoundFiles(SampleBank<float> *bank) {
  using std::filesystem::directory_iterator;
  using std::filesystem::path;
  for (auto &entry : directory_iterator(path{"sounds"})) {
    if (std::filesystem::is_directory(entry))
      continue;

    bank->loadSample(entry.path().string());
  }
}
#endif
