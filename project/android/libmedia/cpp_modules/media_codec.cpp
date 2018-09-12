#include "media_codec.h"
#include <unistd.h>


MediaClassEngine::MediaClassEngine() :
        m_codec_thread(nullptr){
    m_bStop = true;
    m_format = nullptr;
    m_android_media_codec = nullptr;
    m_output_buffer_info = nullptr;
//    m_input_buffer_array = nullptr;
}


MediaClassEngine::~MediaClassEngine() {

    freeVPacket();

    ALOGW("MediaClassEngine delete");
}

int MediaClassEngine::createEnv(JNIEnv **env)
{
    if (JNI_OK != SDL_JNI_SetupThreadEnv(env)) {
        ALOGE("%s: SetupThreadEnv failed\n", __func__);
        return -1;
    }
    return 0;
}

static std::string MIME_TYPE = "video/avc"; // H.264 Advanced Video Coding
static int FRAME_RATE = 30;          // 30fps
static int IFRAME_INTERVAL = 3;      // 1 seconds between I-frames
static int BITRATE = 4 * 1024 * 1024;
static int ControlRateConstant = 2;
static int32_t FormatSurface = 0x7F000789;
static int32_t Format32bitABGR8888 = 0x7F00A000;
static int32_t COLOR_FormatYUV420Flexible = 0x7F420888;     //this maybe nv12
static int CONFIGURE_FLAG_ENCODE = 1;


#define KEY_COLOR_FORMAT    "color-format"
#define KEY_BIT_RATE        "bitrate"
#define KEY_BITRATE_MODE    "bitrate-mode"
#define KEY_FRAME_RATE      "frame-rate"
#define KEY_I_FRAME_INTERVAL "i-frame-interval"



jobject MediaClassEngine::createCodec(JNIEnv *env, int32_t width, int32_t height, bool useSurface) {
    m_use_Surface = useSurface;
    mWidth = width;
    mHeight = height;
    int color_format = COLOR_FormatYUV420Flexible;
    if(m_use_Surface){
        color_format = FormatSurface;
    }
    else{
        color_format = COLOR_FormatYUV420Flexible;
    }

    m_format = J4AC_MediaFormat__createVideoFormat__withCString__asGlobalRef__catchAll(env, MIME_TYPE.c_str(), mWidth, mHeight);
    J4AC_MediaFormat__setInteger__withCString(env, m_format, KEY_BIT_RATE, BITRATE);
    J4AC_MediaFormat__setInteger__withCString(env, m_format, KEY_BITRATE_MODE, ControlRateConstant);
    J4AC_MediaFormat__setInteger__withCString(env, m_format, KEY_COLOR_FORMAT, color_format);
    J4AC_MediaFormat__setInteger__withCString(env, m_format, KEY_FRAME_RATE, FRAME_RATE);
    J4AC_MediaFormat__setInteger__withCString(env, m_format, KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);


    jobject media_codec = J4AC_MediaCodec__createEncoderByType__withCString__catchAll(env, MIME_TYPE.c_str());
    if(media_codec == NULL){
        ALOGE("MediaCodec createByCodecName error");
        return nullptr;
    }
    m_android_media_codec = env->NewGlobalRef(media_codec);
    J4AC_MediaCodec__configure__catchAll(env, m_android_media_codec, m_format, NULL, NULL, CONFIGURE_FLAG_ENCODE);
    SDL_JNI_DeleteLocalRefP(env, &media_codec);
    if(m_use_Surface){
        jobject  surface  = J4AC_MediaCodec__createInputSurface__catchAll(env, m_android_media_codec);
        if(surface == nullptr){
            ALOGE("MediaCodec createInputSurface error");
            return nullptr;
        }
        jobject global_surface = env->NewGlobalRef(surface);
        return global_surface;
    }
    return nullptr;
}

void MediaClassEngine::startEncode(JNIEnv *env)
{

    J4AC_MediaCodec__start__catchAll(env, m_android_media_codec);
    m_output_buffer_info = J4AC_MediaCodec__BufferInfo__BufferInfo__asGlobalRef__catchAll(env);
    //m_input_buffer_array =  J4AC_MediaCodec__getInputBuffers__asGlobalRef__catchAll(env, m_android_media_codec);

    m_codec_thread.reset(new std::thread(&MediaClassEngine::codecProcessThread, this));
    return;
}

void MediaClassEngine::codecProcessThread()
{
    FILE *fd = fopen("/sdcard/Download/jie.h264", "wb");
    JNIEnv *env = nullptr;
    createEnv(&env);
    m_bStop = false;

    size_t size = mWidth * mHeight * 1.5;
    uint8_t *buffer = (uint8_t*)malloc(size);
    FILE *fd_src = fopen("/sdcard/Download/111.yuv", "rb");
    fread(buffer , 1, size, fd_src);
    fclose(fd_src);

    jlong time_stamp = 0;



    while (!m_bStop){
        if(!m_use_Surface){
            writeFrame(env, buffer, size, time_stamp);
            time_stamp  = time_stamp +  1000000/FRAME_RATE;
        }
        for(; ;){
            VPacket packet;
            readPacket(env, &packet);
            if(packet.buffer_size <= 0){
                break;
            }
            fwrite(packet.buffer, 1, packet.buffer_size, fd);
        }
    }
    releaseEncode(env);
    SDL_JNI_DetachThreadEnv();
    ALOGW("encode thread exit");
    free(buffer);
    fclose(fd);
    return;
}

int MediaClassEngine::writeFrame(JNIEnv *env, const uint8_t *buffer, const size_t size, int64_t pts){
    static int TIME_OUT = 10 * 1000;
    int ret = 0;
    int write_index = J4AC_MediaCodec__dequeueInputBuffer__catchAll(env, m_android_media_codec, TIME_OUT);
    if(write_index > 0 ){
//        jobject input_buffer = env->GetObjectArrayElement(m_input_buffer_array, write_index);
        jobject input_buffer = J4AC_MediaCodec__getInputBuffer__catchAll(env, m_android_media_codec, write_index);
        if (J4A_ExceptionCheck__catchAll(env) || !input_buffer) {
            ALOGE("GetObjectArrayElement failed");
            ret = -1;
        }
        size_t write_size = 0;
        {
            jlong buf_size = env->GetDirectBufferCapacity(input_buffer);
            void *buf_ptr  = env->GetDirectBufferAddress(input_buffer);
            write_size = size < buf_size ? size : buf_size;
            memcpy(buf_ptr, buffer, write_size);
        }

        J4AC_MediaCodec__queueInputBuffer__catchAll(env, m_android_media_codec, write_index, 0, write_size, pts, 0);
        SDL_JNI_DeleteLocalRefP(env, &input_buffer);
    }
    return ret;
}

int MediaClassEngine::readPacket(JNIEnv *env, VPacket *packet){
    static int TIME_OUT = 5 * 1000;
    packet->initPacket();
    int ret = 0;
    for( ; ; ){
        int read_index = J4AC_MediaCodec__dequeueOutputBuffer__catchAll(env, m_android_media_codec, m_output_buffer_info, TIME_OUT/10);
        if(read_index >= 0){
            jobject output_buffer = J4AC_MediaCodec__getOutputBuffer__catchAll(env, m_android_media_codec, read_index);
            int flag = J4AC_MediaCodec__BufferInfo__flags__get__catchAll(env, m_output_buffer_info);
            int size = J4AC_MediaCodec__BufferInfo__size__get__catchAll(env, m_output_buffer_info);
            int offset = J4AC_MediaCodec__BufferInfo__offset__get__catchAll(env, m_output_buffer_info);
            int64_t pts = J4AC_MediaCodec__BufferInfo__presentationTimeUs__get__catchAll(env, m_output_buffer_info);
            bool is_configure_frame = (flag & AMEDIACODEC__BUFFER_FLAG_CODEC_CONFIG);
            bool is_key_frame = (flag & AMEDIACODEC__BUFFER_FLAG_SYNC_FRAME);
            if(is_configure_frame){
                jlong buf_size = env->GetDirectBufferCapacity(output_buffer);
                void *buf_ptr  = env->GetDirectBufferAddress(output_buffer);
                m_configure_data.putData((uint8_t*)buf_ptr + offset, size);
                ALOGW("configure_frame  buf_size %lld  offset %d  size %d", buf_size, offset, size);
            }
            else{
                if(m_configure_data.getSize() <= 0){
                    ALOGE("error, no sps pps data get!!!");
                }

                jlong buf_size = env->GetDirectBufferCapacity(output_buffer);
                void *buf_ptr  = env->GetDirectBufferAddress(output_buffer);
                if(is_key_frame){
                    reallocVPacket(size + m_configure_data.getSize());
                    m_temp_packet.buffer_size = size + m_configure_data.getSize();

                    memcpy(m_temp_packet.buffer, m_configure_data.getData(), m_configure_data.getSize());
                    memcpy(m_temp_packet.buffer + m_configure_data.getSize(), (uint8_t*)buf_ptr + offset, size);
                }
                else{
                    reallocVPacket(size);
                    m_temp_packet.buffer_size = size;

                    memcpy(m_temp_packet.buffer, (uint8_t*)buf_ptr + offset, size);
                }
                m_temp_packet.keyFrame = is_key_frame;
                m_temp_packet.pts = pts;
                m_temp_packet.dts = pts;
                *packet = m_temp_packet;
//                ALOGW("h264_frame  buf_size %lld  offset %d  size %d   pts  %lld", buf_size, offset, size, pts);
                ret = 0;
            }
            J4AC_MediaCodec__releaseOutputBuffer__catchAll(env, m_android_media_codec, read_index, 0);
            SDL_JNI_DeleteLocalRefP(env, &output_buffer);
            if(!is_configure_frame){
                break;
            }
        }
        else{
            ret = 0;
            if(read_index == AMEDIACODEC__INFO_TRY_AGAIN_LATER){
//                ALOGE("getOutputBuffer error TRY_AGAIN_LATER");
            }
            else if(read_index == AMEDIACODEC__INFO_OUTPUT_FORMAT_CHANGED){
                ALOGE("getOutputBuffer error FORMAT_CHANGED");
            }
            else if(read_index == AMEDIACODEC__INFO_OUTPUT_BUFFERS_CHANGED){
                ALOGE("getOutputBuffer error BUFFERS_CHANGED");
            }
            else{
                ret = -1;
                ALOGE("getOutputBuffer error");
            }
            break;
        }
    }
    return ret;
}


void MediaClassEngine::stopCodec(JNIEnv *env){
    m_bStop = true;
    if(m_codec_thread){
        m_bStop = true;
        m_codec_thread->join();
        m_codec_thread.reset();
    }
}



void MediaClassEngine::releaseEncode(JNIEnv *env){
    SDL_JNI_DeleteGlobalRefP(env, &m_format);
    SDL_JNI_DeleteGlobalRefP(env, &m_output_buffer_info);
//    SDL_JNI_DeleteGlobalRefP(env, (jobject*)&m_input_buffer_array);
    if(m_android_media_codec){
        J4AC_MediaCodec__stop__catchAll(env, m_android_media_codec);
        J4AC_MediaCodec__release__catchAll(env, m_android_media_codec);
        SDL_JNI_DeleteGlobalRefP(env, &m_android_media_codec);
        m_android_media_codec = nullptr;
    }
}


