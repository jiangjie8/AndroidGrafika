//
// Created by jiangjie on 2018/8/27.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H


#ifdef __cplusplus
#define __EXTERN_C_BEGIN extern "C" {
#define __EXTERN_C_END }
#else
#define  __EXTERN_C_BEGIN
#define __EXTERN_C_END
#endif

#include <string>
#include <inttypes.h>

#if defined(Android)
#include <android/log.h>
#define J4A_LOG_TAG "AndroidGrafika"
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,    J4A_LOG_TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,      J4A_LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,       J4A_LOG_TAG, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,       J4A_LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,      J4A_LOG_TAG, __VA_ARGS__)

#elif defined(Win)

#define LOGV(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#elif defined(Linux)
#include <unistd.h>
#define LOGV(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGW(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(fmt, ...)   printf("jie=[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
 
#endif


#endif //ANDROIDGRAFIKA_MEDIA_STRUCT_H
