//
// Created by jiangjie on 2018/11/13.
//

#ifndef ANDROID_NATIVE_LOG_H
#define ANDROID_NATIVE_LOG_H
#include "jni_bridge.h"

struct JavaLogging {
    jclass classId;
    jmethodID method_debug;
    //jmethodID method_info;
    jmethodID method_warning;
    jmethodID method_error;
};

int J4A_Logging_Class_Init(JNIEnv *env);


void android_logging_debug_print(const char *tag, const char *fmt, ...);
void android_logging_info_print(const char *tag, const char *fmt, ...);
void android_logging_warning_print(const char *tag, const char *fmt, ...);
void android_logging_error_print(const char *tag, const char *fmt, ...);

#endif //ANDROID_NATIVE_LOG_H
