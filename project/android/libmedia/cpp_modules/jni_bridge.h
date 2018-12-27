#ifndef JNI_BRIDGE_H
#define JNI_BRIDGE_H


#ifdef __cplusplus
#define __EXTERN_C_BEGIN extern "C" {
#define __EXTERN_C_END }
#else
#define  __EXTERN_C_BEGIN
#define __EXTERN_C_END
#endif

#include <jni.h>
#include <string>

__EXTERN_C_BEGIN
#include "j4a/j4a_base.h"
#include "jni_bridge.h"
#include "j4a/class/android/os/Build.h"
#include "media_common/native_log.h"
__EXTERN_C_END


#define J4A_LOG_TAG "AndroidGrafika"

//#define EXTRA_LOG_PRINT

#ifdef EXTRA_LOG_PRINT

#define ALOGV(fmt, ...)   android_logging_debug_print(J4A_LOG_TAG, fmt, ##__VA_ARGS__)
#define ALOGD(fmt, ...)   android_logging_debug_print(J4A_LOG_TAG, fmt, ##__VA_ARGS__)
#define ALOGI(fmt, ...)   android_logging_info_print(J4A_LOG_TAG, fmt, ##__VA_ARGS__)
#define ALOGW(fmt, ...)   android_logging_warning_print(J4A_LOG_TAG, fmt, ##__VA_ARGS__)
#define ALOGE(fmt, ...)   android_logging_error_print(J4A_LOG_TAG, fmt, ##__VA_ARGS__)

#else

#define ALOGV(fmt, ...)   J4A_ALOGV(fmt, ##__VA_ARGS__)
#define ALOGD(fmt, ...)   J4A_ALOGD(fmt, ##__VA_ARGS__)
#define ALOGI(fmt, ...)   J4A_ALOGI(fmt, ##__VA_ARGS__)
#define ALOGW(fmt, ...)   J4A_ALOGW(fmt, ##__VA_ARGS__)
#define ALOGE(fmt, ...)   J4A_ALOGE(fmt, ##__VA_ARGS__)

#endif





#endif