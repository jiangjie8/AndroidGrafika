//
// Created by jiangjie on 2018/11/13.
//

#include "jni/android_jni.h"
#include "native_log.h"

constexpr static int LOG_BUF_SIZE = 1024;

static JavaLogging java_class_Logging;

int J4A_Logging_Class_Init(JNIEnv *env){
    int         ret                   = -1;
    const char *J4A_UNUSED(name)      = NULL;
    const char *J4A_UNUSED(sign)      = NULL;
    jclass      J4A_UNUSED(class_id)  = NULL;
    int         J4A_UNUSED(api_level) = 0;

    sign = "apprtc/org/grafika/Logging";
    java_class_Logging.classId = J4A_FindClass__asGlobalRef__catchAll(env, sign);
    if(java_class_Logging.classId == nullptr)
        goto fail;

    class_id = java_class_Logging.classId;
    name = "d";
    sign = "(Ljava/lang/String;Ljava/lang/String;)V";
    java_class_Logging.method_debug = J4A_GetStaticMethodID__catchAll(env, class_id, name, sign);
    if(java_class_Logging.method_debug == nullptr)
        goto fail;

    class_id = java_class_Logging.classId;
    name = "e";
    sign = "(Ljava/lang/String;Ljava/lang/String;)V";
    java_class_Logging.method_error = J4A_GetStaticMethodID__catchAll(env, class_id, name, sign);
    if(java_class_Logging.method_error == nullptr)
        goto fail;

    class_id = java_class_Logging.classId;
    name = "w";
    sign = "(Ljava/lang/String;Ljava/lang/String;)V";
    java_class_Logging.method_warring = J4A_GetStaticMethodID__catchAll(env, class_id, name, sign);
    if(java_class_Logging.method_warring == nullptr)
        goto fail;

    return 0;
    fail:
    return -1;
}

void J4AC_Logging_debug__withCString__catchAll(JNIEnv *env, const char *tag_cstr, const char *message_cstr)
{
    jstring tag = NULL;
    jstring message = NULL;

    tag = env->NewStringUTF(tag_cstr);
    if (J4A_ExceptionCheck__catchAll(env) || !tag)
        goto fail;

    message = env->NewStringUTF(message_cstr);
    if (J4A_ExceptionCheck__catchAll(env) || !message)
        goto fail;

    env->CallStaticVoidMethod(java_class_Logging.classId, java_class_Logging.method_debug, tag, message);

    J4A_ExceptionCheck__catchAll(env);

    fail:
    if(tag)
        env->DeleteLocalRef(tag);
    if(message)
        env->DeleteLocalRef(message);

    return;
}


void J4AC_Logging_warring__withCString__catchAll(JNIEnv *env, const char *tag_cstr, const char *message_cstr)
{
    jstring tag = NULL;
    jstring message = NULL;

    tag = env->NewStringUTF(tag_cstr);
    if (J4A_ExceptionCheck__catchAll(env) || !tag)
        goto fail;

    message = env->NewStringUTF(message_cstr);
    if (J4A_ExceptionCheck__catchAll(env) || !message)
        goto fail;

    env->CallStaticVoidMethod(java_class_Logging.classId, java_class_Logging.method_warring, tag, message);

    J4A_ExceptionCheck__catchAll(env);

    fail:
    if(tag)
        env->DeleteLocalRef(tag);
    if(message)
        env->DeleteLocalRef(message);
    return;
}


void J4AC_Logging_error__withCString__catchAll(JNIEnv *env, const char *tag_cstr, const char *message_cstr)
{
    jstring tag = NULL;
    jstring message = NULL;

    tag = env->NewStringUTF(tag_cstr);
    if (J4A_ExceptionCheck__catchAll(env) || !tag)
        goto fail;

    message = env->NewStringUTF(message_cstr);
    if (J4A_ExceptionCheck__catchAll(env) || !message)
        goto fail;

    env->CallStaticVoidMethod(java_class_Logging.classId, java_class_Logging.method_error, tag, message);

    J4A_ExceptionCheck__catchAll(env);

    fail:
    if(tag)
        env->DeleteLocalRef(tag);
    if(message)
        env->DeleteLocalRef(message);
    return;
}

void android_logging_debug_print(const char *tag, const char *fmt, ...){

    JNIEnv *env = NULL;

    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        return;
    }
    va_list ap;
    char log_buffer[LOG_BUF_SIZE + 1] = {0};

    va_start(ap, fmt);
    vsnprintf(log_buffer, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    J4AC_Logging_debug__withCString__catchAll(env, tag, log_buffer);
}

void android_logging_info_print(const char *tag, const char *fmt, ...){

    JNIEnv *env = NULL;

    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        return;
    }
    va_list ap;
    char log_buffer[LOG_BUF_SIZE + 1] = {0};

    va_start(ap, fmt);
    vsnprintf(log_buffer, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    J4AC_Logging_debug__withCString__catchAll(env, tag, log_buffer);
}

void android_logging_warring_print(const char *tag, const char *fmt, ...){

    JNIEnv *env = NULL;

    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        return;
    }
    va_list ap;
    char log_buffer[LOG_BUF_SIZE + 1] = {0};

    va_start(ap, fmt);
    vsnprintf(log_buffer, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    J4AC_Logging_warring__withCString__catchAll(env, tag, log_buffer);
}

void android_logging_error_print(const char *tag, const char *fmt, ...){

    JNIEnv *env = NULL;

    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        return;
    }
    va_list ap;
    char log_buffer[LOG_BUF_SIZE + 1] = {0};

    va_start(ap, fmt);
    vsnprintf(log_buffer, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    J4AC_Logging_error__withCString__catchAll(env, tag, log_buffer);
}