#include <string>
#include <pthread.h>
#include <unistd.h>
#include "jni_bridge.h"
#include "media_codec.h"
#include "android_jni.h"
#include "audio_playback.h"
#include "ff_recode.h"

JNIEXPORT jlong JNICALL createEngine_mediacodec(JNIEnv *env, jclass) {
    MediaClassEngine *codec_engine = new MediaClassEngine();
    int64_t engineHandle = reinterpret_cast<int64_t>(codec_engine);
    return (jlong) engineHandle;
}

JNIEXPORT jobject JNICALL configureCodec_mediacodec(JNIEnv *env, jclass clazz, jlong engineHandle, jint width, jint height, jboolean userSurface) {
    MediaClassEngine *codec_engine = reinterpret_cast<MediaClassEngine*>(engineHandle);
    jobject surface = codec_engine->createCodec(env, width, height, userSurface);
    return surface;
}

JNIEXPORT void JNICALL startCodec_mediacodec(JNIEnv *env, jclass clazz, jlong engineHandle) {
    MediaClassEngine *codec_engine = reinterpret_cast<MediaClassEngine*>(engineHandle);
    codec_engine->startEncode(env);
}


JNIEXPORT void JNICALL stopCodec_mediacodec(JNIEnv *env, jclass clazz, jlong engineHandle) {
    MediaClassEngine *codec_engine = reinterpret_cast<MediaClassEngine*>(engineHandle);
    codec_engine->stopCodec(env);
}

JNIEXPORT void JNICALL deleteEngine_mediacodec(JNIEnv *env, jclass clazz, jlong engineHandle) {
    MediaClassEngine *codec_engine = reinterpret_cast<MediaClassEngine*>(engineHandle);
    delete codec_engine;
}


JNIEXPORT jlong JNICALL createEngine_audioOboe(JNIEnv *env, jclass) {
    AudioPalybackEngine *engine = new(std::nothrow) AudioPalybackEngine();
    int64_t engineHandle = reinterpret_cast<int64_t>(engine);
    return engineHandle;
}

JNIEXPORT void JNICALL setToneOn_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle, jboolean isToneOn) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);

    return;
}

JNIEXPORT void JNICALL setAudioApi_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle, jint audioApi) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    oboe::AudioApi api = static_cast<oboe::AudioApi>(audioApi);
    engine->setAudioApi(api);
    return;
}

JNIEXPORT void JNICALL setAudioDeviceId_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle, jint deviceId) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    engine->setDeviceId(deviceId);
    return;
}

JNIEXPORT void JNICALL setChannelCount_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle, jint channelCount) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    engine->setChannelCount(channelCount);
    return;
}

JNIEXPORT void JNICALL setBufferSizeInBursts_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle, jint bufferSizeInBursts) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    engine->setBufferSizeInBursts(bufferSizeInBursts);
    return;
}

JNIEXPORT jdouble JNICALL getCurrentOutputLatencyMillis_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    return engine->getCurrentOutputLatencyMillis();
}

JNIEXPORT jboolean JNICALL isLatencyDetectionSupported_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    return engine->isLatencyDetectionSupported();
}

JNIEXPORT void JNICALL deleteEngine_audioOboe(JNIEnv *env, jclass clazz, jlong engineHandle) {
    AudioPalybackEngine *engine = reinterpret_cast<AudioPalybackEngine*>(engineHandle);
    delete engine;
}


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


JNIEXPORT jint JNICALL demuxer_openOutputFormat(JNIEnv *env, jclass clazz, jlong engineHandle, jstring outputStr) {
    const char *c_name = env->GetStringUTFChars(outputStr, 0);
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    int ret = engine->openOutputFormat(env, c_name);
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
    return engine->readPacket(env, packet);
}

JNIEXPORT jint JNICALL demuxer_writePacket(JNIEnv *env, jclass clazz, jlong engineHandle, jobject packet) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    return engine->writePacket(env, packet);
}


JNIEXPORT jint JNICALL demuxer_getMediaInfo(JNIEnv *env, jclass clazz, jlong engineHandle, jobject packet) {
    av::FFRecoder *engine = reinterpret_cast<av::FFRecoder*>(engineHandle);
    return engine->probeMediaInfo(env, packet);
}


static JNINativeMethod g_methods[] = {
        {"native_createEngine_mediacodec",         "()J",          (void *) createEngine_mediacodec},
        {"native_configureCodec_mediacodec",       "(JIIZ)Landroid/view/Surface;",         (void *) configureCodec_mediacodec},
        {"native_startCodec_mediacodec",           "(J)V",         (void *) startCodec_mediacodec},
        {"native_stopCodec_mediacodec",            "(J)V",         (void *) stopCodec_mediacodec},
        {"native_deleteEngine_mediacodec",         "(J)V",         (void *) deleteEngine_mediacodec},

        {"native_createEngine_audioOboe",               "()J",              (void *) createEngine_audioOboe},
        {"native_deleteEngine_audioOboe",               "(J)V",             (void *) deleteEngine_audioOboe},
        {"native_setToneOn_audioOboe",                  "(JZ)V",            (void *) setToneOn_audioOboe},
        {"native_setAudioApi_audioOboe",                "(JI)V",            (void *) setAudioApi_audioOboe},
        {"native_setAudioDeviceId_audioOboe",           "(JI)V",            (void *) setAudioDeviceId_audioOboe},
        {"native_setChannelCount_audioOboe",            "(JI)V",            (void *) setChannelCount_audioOboe},
        {"native_setBufferSizeInBursts_audioOboe",      "(JI)V",            (void *) setBufferSizeInBursts_audioOboe},
        {"native_getCurrentOutputLatencyMillis_audioOboe",      "(J)D",          (void *) getCurrentOutputLatencyMillis_audioOboe},
        {"native_isLatencyDetectionSupported_audioOboe",        "(J)Z",          (void *) isLatencyDetectionSupported_audioOboe},


        {"native_demuxer_createEngine",                 "()J",  (void *) demuxer_createEngine},
        {"native_demuxer_openInputFormat",              "(JLjava/lang/String;)I",   (void *) demuxer_openInputFormat},
        {"native_demuxer_closeInputFormat",              "(J)I",   (void *) demuxer_closeInputFormat},
        {"native_demuxer_openOutputFormat",              "(JLjava/lang/String;)I",   (void *) demuxer_openOutputFormat},
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
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
}



