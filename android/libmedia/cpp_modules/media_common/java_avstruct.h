//
// Created by jiangjie on 2018/8/24.
//

#ifndef ANDROIDGRAFIKA_JAVA_AVSTRUCT_H
#define ANDROIDGRAFIKA_JAVA_AVSTRUCT_H

#include "jni_bridge.h"


struct JavaAVPacket {
    jclass classId;
    jmethodID constructor_AVPacket;

    jfieldID  filed_buffer;
    jfieldID  filed_bufferSize;
    jfieldID  filed_bufferOffset;
    jfieldID  filed_bufferFlags;
    jfieldID  filed_mediaType;
    jfieldID  filed_ptsUs;
    jfieldID  filed_dtsUs;
};



int J4A_AVPacket_Class_Init(JNIEnv *env);

int J4A_set_avpacket(JNIEnv *env, jobject object,  const void *buffer, jint size, jlong ptsUs, jlong dtsUs, jint mediaType);
int J4A_AVPacket_setFiled_ptsUs(JNIEnv *env, jobject object, jlong pts);
int64_t J4A_AVPacket_getFiled_ptsUs(JNIEnv *env, jobject object);
int J4A_AVPacket_setFiled_dtsUs(JNIEnv *env, jobject object, jlong dts);
int64_t J4A_AVPacket_getFiled_dtsUs(JNIEnv *env, jobject object);
int J4A_AVPacket_setFiled_mediaType(JNIEnv *env, jobject object, int mediaType);
int32_t J4A_AVPacket_getFiled_mediaType(JNIEnv *env, jobject object);
int J4A_AVPacket_setFiled_bufferSize(JNIEnv *env, jobject object, int bufferSize);
int32_t J4A_AVPacket_getFiled_bufferSize(JNIEnv *env, jobject object);
int J4A_AVPacket_setFiled_bufferOffset(JNIEnv *env, jobject object, int bufferOffset);
int32_t J4A_AVPacket_getFiled_bufferOffset(JNIEnv *env, jobject object);
int J4A_AVPacket_setFiled_bufferFlags(JNIEnv *env, jobject object, int bufferFlags);
int32_t J4A_AVPacket_getFiled_bufferFlags(JNIEnv *env, jobject object);
jobject J4A_AVPacket_getFiled_buffer(JNIEnv *env, jobject object);



struct JavaMediaInfo {
    jclass classId;
    jmethodID constructor_MediaInfo;

    jfieldID  filed_bitrate;
    jfieldID  filed_duration;

    jfieldID  filed_videoCodecID;
    jfieldID  filed_width;
    jfieldID  filed_height;
    jfieldID  filed_framerate;

    jfieldID  filed_audioCodecID;
    jfieldID  filed_channels;
    jfieldID  filed_sampleRate;
    jfieldID  filed_sampleDepth;
    jfieldID  filed_frameSize;
};
jobject J4A_MediaInfo_construct_asGlobalRef_catchAll(JNIEnv *env);
int J4A_MediaInfo_Class_Init(JNIEnv *env);
int J4A_MediaInfo_setFiled_bitrate(JNIEnv *env, jobject object, int bitrate);
int J4A_MediaInfo_setFiled_duration(JNIEnv *env, jobject object, long duration);
int J4A_MediaInfo_setFiled_videoCodecID(JNIEnv *env, jobject object, int name);
int J4A_MediaInfo_setFiled_width(JNIEnv *env, jobject object, int width);
int J4A_MediaInfo_setFiled_height(JNIEnv *env, jobject object, int height);
int J4A_MediaInfo_setFiled_framerate(JNIEnv *env, jobject object, int framerate);
int J4A_MediaInfo_setFiled_audioCodecID(JNIEnv *env, jobject object, int name);
int J4A_MediaInfo_setFiled_channels(JNIEnv *env, jobject object, int channels);
int J4A_MediaInfo_setFiled_sampleRate(JNIEnv *env, jobject object, int sampleRate);
int J4A_MediaInfo_setFiled_sampleDepth(JNIEnv *env, jobject object, int sampleDepth);
int J4A_MediaInfo_setFiled_frameSize(JNIEnv *env, jobject object, int frameSize);

#endif //ANDROIDGRAFIKA_JAVA_AVSTRUCT_H
