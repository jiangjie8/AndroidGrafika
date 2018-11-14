#include <string>
#include <pthread.h>
#include <unistd.h>
#include "jni_bridge.h"
#include "jni/android_jni.h"
#include "ff_recode.h"


JNIEXPORT jlong JNICALL demuxer_createEngine(JNIEnv *env, jclass clazz) {
    av::FFRecoder *demuxer_engine = new av::FFRecoder();
    int64_t engineHandle = reinterpret_cast<int64_t>(demuxer_engine);
    return (jlong) engineHandle;
}

JNIEXPORT jint JNICALL demuxer_closeInputFormat(JNIEnv *env, jclass clazz, jlong engineHandle) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    engine->closeInputFormat();
    delete engine;
    return 0;
}

JNIEXPORT jint JNICALL demuxer_openInputFormat(JNIEnv *env, jclass clazz, jlong engineHandle, jstring inputStr) {
    const char *c_name = env->GetStringUTFChars(inputStr, 0);
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    int ret = engine->openInputFormat(env, c_name);
    if(c_name)
        env->ReleaseStringUTFChars(inputStr, c_name);
    return ret;
}


JNIEXPORT jint JNICALL demuxer_openOutputFormat(JNIEnv *env, jclass clazz, jlong engineHandle, jstring outputStr, jobject mediaInfo) {
    const char *c_name = env->GetStringUTFChars(outputStr, 0);
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    int ret = engine->openOutputFormat(env, c_name, mediaInfo);
    if(c_name)
        env->ReleaseStringUTFChars(outputStr, c_name);
    return ret;
}

JNIEXPORT jint JNICALL demuxer_closeOutputFormat(JNIEnv *env, jclass clazz, jlong engineHandle) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    return engine->closeOutputFormat();
}

JNIEXPORT jint JNICALL demuxer_readPacket(JNIEnv *env, jclass clazz, jlong engineHandle, jobject packet) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    return engine->readVideoPacket(env, packet);
}

JNIEXPORT jint JNICALL demuxer_writePacket(JNIEnv *env, jclass clazz, jlong engineHandle, jobject packet) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    return engine->writePacket(env, packet);
}


JNIEXPORT jint JNICALL demuxer_getMediaInfo(JNIEnv *env, jclass clazz, jlong engineHandle, jobject mediaInfo) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    return engine->getMediaInfo(env, mediaInfo);
}


static JNINativeMethod g_methods[] = {
        {"native_demuxer_createEngine",                 "()J",  (void *) demuxer_createEngine},
        {"native_demuxer_openInputFormat",              "(JLjava/lang/String;)I",   (void *) demuxer_openInputFormat},
        {"native_demuxer_closeInputFormat",              "(J)I",   (void *) demuxer_closeInputFormat},
        {"native_demuxer_openOutputFormat",              "(JLjava/lang/String;Lapprtc/org/grafika/media/AVStruct$MediaInfo;)I",   (void *) demuxer_openOutputFormat},
        {"native_demuxer_closeOutputFormat",              "(J)I",   (void *) demuxer_closeOutputFormat},
        {"native_demuxer_readPacket",                   "(JLapprtc/org/grafika/media/AVStruct$AVPacket;)I",   (void *) demuxer_readPacket},
        {"native_demuxer_writePacket",                  "(JLapprtc/org/grafika/media/AVStruct$AVPacket;)I",   (void *) demuxer_writePacket},
        {"native_demuxer_getMediaInfo",                 "(JLapprtc/org/grafika/media/AVStruct$MediaInfo;)I",  (void *) demuxer_getMediaInfo},
};




typedef struct player_fields_t {
    pthread_mutex_t mutex;
    jclass clazz;
} player_fields_t;
static player_fields_t g_clazz;

#define JNI_CLASS_JNIBridge     "apprtc/org/grafika/JNIBridge"




JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    int retval = 0;
    JNIEnv* env = NULL;
    SDL_JNI_SetJVM(vm);
    //g_jvm = vm;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);
    pthread_mutex_init(&(g_clazz.mutex), NULL);
    IJK_FIND_JAVA_CLASS(env, g_clazz.clazz, JNI_CLASS_JNIBridge);
    env->RegisterNatives(g_clazz.clazz, g_methods, (sizeof(g_methods)/sizeof(g_methods[0])));

    retval = J4A_LoadAll__catchAll(env);
    JNI_CHECK_RET(retval == 0, env, NULL, NULL, -1);

    {
        J4A_Logging_Class_Init(env);
        J4A_AVPacket_Class_Init(env);
        J4A_MediaInfo_Class_Init(env);
    }

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
}



