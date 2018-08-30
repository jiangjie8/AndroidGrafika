//
// Created by jiangjie on 2018/8/24.
//

#include "java_avstruct.h"


static JavaAVPacket java_class_AVPacket;
static JavaMediaInfo java_class_MediaInfo;

int J4A_AVPacket_Class_Init(JNIEnv *env){
    int         ret                   = -1;
    const char *J4A_UNUSED(name)      = NULL;
    const char *J4A_UNUSED(sign)      = NULL;
    jclass      J4A_UNUSED(class_id)  = NULL;
    int         J4A_UNUSED(api_level) = 0;

    sign = "apprtc/org/grafika/media/AVStruct$AVPacket";
    java_class_AVPacket.classId = J4A_FindClass__asGlobalRef__catchAll(env, sign);
    if(java_class_AVPacket.classId == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name     = "<init>";
    sign     = "()V";
    java_class_AVPacket.constructor_AVPacket = J4A_GetMethodID__catchAll(env, class_id, name, sign);
    if (java_class_AVPacket.constructor_AVPacket == NULL)
        goto fail;


    class_id = java_class_AVPacket.classId;
    name = "buffer";
    sign = "Ljava/nio/ByteBuffer;";
    java_class_AVPacket.filed_buffer = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_buffer == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name = "bufferSize";
    sign = "I";
    java_class_AVPacket.filed_bufferSize = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_bufferSize == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name = "bufferOffset";
    sign = "I";
    java_class_AVPacket.filed_bufferOffset = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_bufferOffset == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name = "bufferFlags";
    sign = "I";
    java_class_AVPacket.filed_bufferFlags = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_bufferFlags == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name = "mediaType";
    sign = "I";
    java_class_AVPacket.filed_mediaType = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_mediaType == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name = "ptsUs";
    sign = "J";
    java_class_AVPacket.filed_ptsUs = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_ptsUs == nullptr)
        goto fail;

    class_id = java_class_AVPacket.classId;
    name = "dtsUs";
    sign = "J";
    java_class_AVPacket.filed_dtsUs = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_AVPacket.filed_dtsUs == nullptr)
        goto fail;

    return 0;
    fail:
    return -1;
}



int J4A_AVPacket_setFiled_ptsUs(JNIEnv *env, jobject object, jlong pts){
    env->SetLongField(object, java_class_AVPacket.filed_ptsUs, pts);
    return 0;
}
int64_t J4A_AVPacket_getFiled_ptsUs(JNIEnv *env, jobject object){
    return env->GetLongField(object, java_class_AVPacket.filed_ptsUs);
    return 0;
}
int J4A_AVPacket_setFiled_dtsUs(JNIEnv *env, jobject object, jlong dts){
    env->SetLongField(object, java_class_AVPacket.filed_dtsUs, dts);
    return 0;
}
int64_t J4A_AVPacket_getFiled_dtsUs(JNIEnv *env, jobject object){
    return env->GetLongField(object, java_class_AVPacket.filed_dtsUs);
}
int J4A_AVPacket_setFiled_mediaType(JNIEnv *env, jobject object, int mediaType){
    env->SetIntField(object, java_class_AVPacket.filed_mediaType, mediaType);
    return 0;
}
int32_t J4A_AVPacket_getFiled_mediaType(JNIEnv *env, jobject object){
    return env->GetIntField(object, java_class_AVPacket.filed_mediaType);
}
int J4A_AVPacket_setFiled_bufferSize(JNIEnv *env, jobject object, int bufferSize){
    env->SetIntField(object, java_class_AVPacket.filed_bufferSize, bufferSize);
    return 0;
}
int32_t J4A_AVPacket_getFiled_bufferSize(JNIEnv *env, jobject object){
    return env->GetIntField(object, java_class_AVPacket.filed_bufferSize);
}
int J4A_AVPacket_setFiled_bufferOffset(JNIEnv *env, jobject object, int bufferOffset){
    env->SetIntField(object, java_class_AVPacket.filed_bufferOffset, bufferOffset);
    return 0;
}
int32_t J4A_AVPacket_getFiled_bufferOffset(JNIEnv *env, jobject object){
    return env->GetIntField(object, java_class_AVPacket.filed_bufferOffset);
}
int J4A_AVPacket_setFiled_bufferFlags(JNIEnv *env, jobject object, int bufferFlags){
    env->SetIntField(object, java_class_AVPacket.filed_bufferFlags, bufferFlags);
    return 0;
}
int32_t J4A_AVPacket_getFiled_bufferFlags(JNIEnv *env, jobject object){
    return env->GetIntField(object, java_class_AVPacket.filed_bufferFlags);
}

jobject J4A_AVPacket_getFiled_buffer(JNIEnv *env, jobject object){
    return env->GetObjectField(object, java_class_AVPacket.filed_buffer);
}

int J4A_set_avpacket(JNIEnv *env, jobject object,  const void *buffer, jint size, jlong ptsUs, jlong dtsUs, jint mediaType){
    J4A_AVPacket_setFiled_bufferFlags(env, object, 0);
    J4A_AVPacket_setFiled_bufferOffset(env, object, 0);
    J4A_AVPacket_setFiled_ptsUs(env, object, ptsUs);
    J4A_AVPacket_setFiled_dtsUs(env, object, dtsUs);
    J4A_AVPacket_setFiled_mediaType(env, object, mediaType);

    jobject  bytebuffer = env->GetObjectField(object, java_class_AVPacket.filed_buffer);
    if(bytebuffer == nullptr)
        return -1;
    jint buf_size = env->GetDirectBufferCapacity(bytebuffer);
    void *buf_ptr  = env->GetDirectBufferAddress(bytebuffer);
    if(buf_ptr == nullptr){
        return -1;
    }
    buf_size = size > buf_size ? buf_size : size;
    J4A_AVPacket_setFiled_bufferSize(env, object, buf_size);
    if(buf_size > 0)
        memcpy(buf_ptr, buffer, buf_size);
    return 0;
}


//////////////////////////////////////////////////////////////

int J4A_MediaInfo_Class_Init(JNIEnv *env){

    int         ret                   = -1;
    const char *J4A_UNUSED(name)      = NULL;
    const char *J4A_UNUSED(sign)      = NULL;
    jclass      J4A_UNUSED(class_id)  = NULL;
    int         J4A_UNUSED(api_level) = 0;

    sign = "apprtc/org/grafika/media/AVStruct$MediaInfo";
    java_class_MediaInfo.classId = J4A_FindClass__asGlobalRef__catchAll(env, sign);
    if(java_class_MediaInfo.classId == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name     = "<init>";
    sign     = "()V";
    java_class_MediaInfo.constructor_MediaInfo = J4A_GetMethodID__catchAll(env, class_id, name, sign);
    if (java_class_MediaInfo.constructor_MediaInfo == NULL)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "bitrate";
    sign = "I";
    java_class_MediaInfo.filed_bitrate = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_bitrate == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "duration";
    sign = "J";
    java_class_MediaInfo.filed_duration = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_duration == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "videoCodecID";
    sign = "I";
    java_class_MediaInfo.filed_videoCodecID = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_videoCodecID == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "width";
    sign = "I";
    java_class_MediaInfo.filed_width = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_width == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "height";
    sign = "I";
    java_class_MediaInfo.filed_height = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_height == nullptr)
        goto fail;


    class_id = java_class_MediaInfo.classId;
    name = "framerate";
    sign = "I";
    java_class_MediaInfo.filed_framerate = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_framerate == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "audioCodecID";
    sign = "I";
    java_class_MediaInfo.filed_audioCodecID = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_audioCodecID == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "channels";
    sign = "I";
    java_class_MediaInfo.filed_channels = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_channels == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "sampleRate";
    sign = "I";
    java_class_MediaInfo.filed_sampleRate = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_sampleRate == nullptr)
        goto fail;


    class_id = java_class_MediaInfo.classId;
    name = "sampleDepth";
    sign = "I";
    java_class_MediaInfo.filed_sampleDepth = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_sampleDepth == nullptr)
        goto fail;

    class_id = java_class_MediaInfo.classId;
    name = "frameSize";
    sign = "I";
    java_class_MediaInfo.filed_frameSize = J4A_GetFieldID__catchAll(env, class_id, name, sign);
    if(java_class_MediaInfo.filed_frameSize == nullptr)
        goto fail;

    return 0;
    fail:
    return -1;
}


jobject J4A_MediaInfo_construct_asGlobalRef_catchAll(JNIEnv *env){
    jobject ret_object   = nullptr;
    jobject local_object = env->NewObject(java_class_MediaInfo.classId, java_class_MediaInfo.constructor_MediaInfo);
    if (J4A_ExceptionCheck__catchAll(env) || !local_object) {
        ret_object = nullptr;
        goto fail;
    }

    ret_object = J4A_NewGlobalRef__catchAll(env, local_object);
    if (!ret_object) {
        ret_object = nullptr;
        goto fail;
    }

    fail:
    J4A_DeleteLocalRef__p(env, &local_object);
    return ret_object;

}


int J4A_MediaInfo_setFiled_bitrate(JNIEnv *env, jobject object, int bitrate){
    env->SetIntField(object, java_class_MediaInfo.filed_bitrate, bitrate);
    return 0;
}
int J4A_MediaInfo_setFiled_duration(JNIEnv *env, jobject object, long duration){
    env->SetLongField(object, java_class_MediaInfo.filed_duration, duration);
    return 0;
}
int J4A_MediaInfo_setFiled_videoCodecID(JNIEnv *env, jobject object, int id){
    env->SetIntField(object, java_class_MediaInfo.filed_videoCodecID, id);
    return 0;
}
int J4A_MediaInfo_setFiled_width(JNIEnv *env, jobject object, int width){
    env->SetIntField(object, java_class_MediaInfo.filed_width, width);
    return 0;
}
int J4A_MediaInfo_setFiled_height(JNIEnv *env, jobject object, int height){
    env->SetIntField(object, java_class_MediaInfo.filed_height, height);
    return 0;
}
int J4A_MediaInfo_setFiled_framerate(JNIEnv *env, jobject object, int framerate){
    env->SetIntField(object, java_class_MediaInfo.filed_framerate, framerate);
    return 0;
}
int J4A_MediaInfo_setFiled_audioCodecID(JNIEnv *env, jobject object, int id){
    env->SetIntField(object, java_class_MediaInfo.filed_audioCodecID, id);
    return 0;
}
int J4A_MediaInfo_setFiled_channels(JNIEnv *env, jobject object, int channels){
    env->SetIntField(object, java_class_MediaInfo.filed_channels, channels);
    return 0;
}
int J4A_MediaInfo_setFiled_sampleRate(JNIEnv *env, jobject object, int sampleRate){
    env->SetIntField(object, java_class_MediaInfo.filed_sampleRate, sampleRate);
    return 0;
}
int J4A_MediaInfo_setFiled_sampleDepth(JNIEnv *env, jobject object, int sampleDepth){
    env->SetIntField(object, java_class_MediaInfo.filed_sampleDepth, sampleDepth);
    return 0;
}
int J4A_MediaInfo_setFiled_frameSize(JNIEnv *env, jobject object, int frameSize){
    env->SetIntField(object, java_class_MediaInfo.filed_frameSize, frameSize);
    return 0;
}