//
// Created by jiangjie on 2018/8/27.
//

#ifndef COMMON_H
#define COMMON_H


#ifdef __cplusplus
#define __EXTERN_C_BEGIN extern "C" {
#define __EXTERN_C_END }
#else
#define  __EXTERN_C_BEGIN
#define __EXTERN_C_END
#endif

#include <string>
#include <inttypes.h>

#ifdef Android
#include "jni_bridge.h"
#define LOGV(fmt, ...)   ALOGV(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)   ALOGV(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...)   ALOGV(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...)   ALOGV(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)   ALOGV(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif


#ifdef Win
#define LOGV(fmt, ...)   printf(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)   printf(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...)   printf(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...)   printf(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)   printf(fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif




#endif //ANDROIDGRAFIKA_MEDIA_STRUCT_H
