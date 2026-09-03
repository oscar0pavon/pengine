#ifndef PE_LOG_H
#define PE_LOG_H

#ifdef ANDROID
    #include <android_native_app_glue.h>
    #include <android/log.h>

    #define APP_NAME "pavonstudios"
    #define LOG(...) ((void)__android_log_print(ANDROID_LOG_ERROR, APP_NAME, __VA_ARGS__))
    #define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, APP_NAME, __VA_ARGS__))
    #define ALOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, APP_NAME, __VA_ARGS__))
    #define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, APP_NAME, __VA_ARGS__))
#else
    #include <stdio.h>
    #define LOG printf
#endif//DEF ANDROID

#define LOGW(t) printf(t); printf("\n")

#endif // !PE_LOG_H
