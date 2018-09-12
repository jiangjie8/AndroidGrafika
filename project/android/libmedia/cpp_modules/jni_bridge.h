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
__EXTERN_C_END

#define ALOGV(fmt, ...)   J4A_ALOGV("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ALOGD(fmt, ...)   J4A_ALOGD("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ALOGI(fmt, ...)   J4A_ALOGI("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ALOGW(fmt, ...)   J4A_ALOGW("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ALOGE(fmt, ...)   J4A_ALOGE("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)



#endif