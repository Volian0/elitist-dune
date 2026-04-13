#pragma once

#ifdef __ANDROID__
#include <SDL3/SDL.h>
#include <jni.h>
#endif

#include <atomic>
#include <chrono>
#include <string>

inline std::atomic_bool rewarded=false;
inline std::chrono::steady_clock::time_point last_show_android_ad_click{};

#ifdef __ANDROID__
inline void ShowAndroidAd() {
    const auto tp_now = std::chrono::steady_clock::now();
    if (tp_now - last_show_android_ad_click < std::chrono::seconds(3))
    {
        return;
    }
    last_show_android_ad_click = tp_now;
    // 1. Get the JNI Environment and the Activity context from SDL
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // 2. Get the class of your activity
    jclass clazz = env->GetObjectClass(activity);

    // 3. Get the Method ID of "triggerShowAd" 
    // The signature "()V" means: takes no arguments, returns void (V)
    jmethodID methodID = env->GetMethodID(clazz, "triggerShowAd", "()V");

    if (methodID) {
        // 4. Call the method on the activity instance
        env->CallVoidMethod(activity, methodID);
    }

    // 5. Clean up local references
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(activity);
}

inline void ShowAndroidIAd() {
    const auto tp_now = std::chrono::steady_clock::now();
    // 1. Get the JNI Environment and the Activity context from SDL
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // 2. Get the class of your activity
    jclass clazz = env->GetObjectClass(activity);

    // 3. Get the Method ID of "triggerShowAd" 
    // The signature "()V" means: takes no arguments, returns void (V)
    jmethodID methodID = env->GetMethodID(clazz, "triggerShowIAd", "()V");

    if (methodID) {
        // 4. Call the method on the activity instance
        env->CallVoidMethod(activity, methodID);
    }

    // 5. Clean up local references
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(activity);
}

inline void LoadAndroidAd() {
    const auto tp_now = std::chrono::steady_clock::now();
    // 1. Get the JNI Environment and the Activity context from SDL
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();

    // 2. Get the class of your activity
    jclass clazz = env->GetObjectClass(activity);

    // 3. Get the Method ID of "triggerShowAd" 
    // The signature "()V" means: takes no arguments, returns void (V)
    jmethodID methodID = env->GetMethodID(clazz, "triggerLoadAd", "()V"); 

    if (methodID) {
        // 4. Call the method on the activity instance
        env->CallVoidMethod(activity, methodID);
    }

    // 5. Clean up local references
    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(activity);
}

inline void ShowLeaderboard(const char* t_id)
{
JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz = env->GetObjectClass(activity);

    // Signature "(Ljava/lang/String;)V" means: 
    // Takes one String argument, returns void (V)
    jmethodID methodID = env->GetMethodID(clazz, "ShowLeaderboard", "(Ljava/lang/String;)V");

    if (methodID) {
        // Convert C++ string to Java String
        jstring j_id = env->NewStringUTF(t_id);
        
        // Pass the jstring as an argument
        env->CallVoidMethod(activity, methodID, j_id);
        
        // Clean up the string reference
        env->DeleteLocalRef(j_id);
    }

    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(activity);
}

inline void SubmitLeaderBoard(const char* t_id, long long t_score)
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    jclass clazz = env->GetObjectClass(activity);

    // Signature "(Ljava/lang/String;J)V" means:
    // Takes (String, long) and returns void (V)
    jmethodID methodID = env->GetMethodID(clazz, "SubmitLeaderboard", "(Ljava/lang/String;J)V");

    if (methodID) {
        jstring j_id = env->NewStringUTF(t_id);
        
        // Call with BOTH parameters. 
        // Note: Java "long" is 64-bit, so use jlong in C++
        env->CallVoidMethod(activity, methodID, j_id, (jlong)t_score);
        
        env->DeleteLocalRef(j_id);
    }

    env->DeleteLocalRef(clazz);
    env->DeleteLocalRef(activity);
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_volian_elitistdune_ElitistDune_rewardUser(JNIEnv* env, jobject thiz) {
    rewarded = true;
} 

}
#else
inline void ShowAndroidAd() {
    rewarded = true;
}
inline void LoadAndroidAd()
{
}
inline void ShowAndroidIAd()
{
}
inline void ShowLeaderboard(const char* t_id)
{
}
inline void SubmitLeaderBoard(const char* t_id, long long t_score)
{
}
#endif