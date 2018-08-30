#ifndef MEDIA__CODEC_H
#define MEDIA__CODEC_H

#include "jni_bridge.h"
#include "android_jni.h"
__EXTERN_C_BEGIN
#include "j4a/j4a_base.h"
#include "j4a/class/android/media/MediaCodec.h"
#include "j4a/class/android/media/MediaFormat.h"
__EXTERN_C_END

#include <string>
#include <thread>
#include <memory>
#include <functional>
#include <atomic>


enum {
    AMEDIACODEC__INFO_OUTPUT_BUFFERS_CHANGED = -3,
    AMEDIACODEC__INFO_OUTPUT_FORMAT_CHANGED  = -2,
    AMEDIACODEC__INFO_TRY_AGAIN_LATER        = -1,

    AMEDIACODEC__UNKNOWN_ERROR               = -1000,
};

enum {
    AMEDIACODEC__BUFFER_FLAG_KEY_FRAME       = 0x01,
    AMEDIACODEC__BUFFER_FLAG_SYNC_FRAME      = 0x01,
    AMEDIACODEC__BUFFER_FLAG_CODEC_CONFIG    = 0x02,
    AMEDIACODEC__BUFFER_FLAG_END_OF_STREAM   = 0x04,
    // extended
            AMEDIACODEC__BUFFER_FLAG_FAKE_FRAME      = 0x1000,
};


typedef struct ConfigData{
    ConfigData(){
        m_size = 0;
        m_capacity = 0;
        m_buffer = nullptr;
        m_free_function = [](uint8_t * ptr){
            if(ptr){
                delete [] ptr;
            }
        };
    }
    ~ConfigData(){
        m_size = 0;
        m_capacity = 0;
        m_buffer.reset();
    }
    void putData(const uint8_t *src_buff, size_t src_size){
        if(src_size <= 0  || src_buff == nullptr)
            return;
        if(m_capacity < src_size ||
                m_capacity > src_size + 100){
            m_capacity = src_size;
            m_buffer.reset(new uint8_t[m_capacity], m_free_function);
        }
        m_size = src_size;
        memcpy(m_buffer.get(), src_buff, src_size);
    }

    const uint8_t* getData(){
        return m_buffer.get();
    }

    size_t getSize(){
        return m_size;
    }

private:
    std::shared_ptr<uint8_t> m_buffer;
    size_t m_size;
    size_t m_capacity;
    std::function<void(uint8_t *)> m_free_function;
} ConfigData;


typedef  struct VPacket{
    int64_t  pts;  //The presentation timestamp in microseconds for the buffer.
    int64_t  dts;
    bool keyFrame;
    uint8_t *buffer;
    size_t  buffer_size;
    size_t capacity;

    VPacket(){
        initPacket();
    }

    void initPacket(){
        pts = 0;
        dts = 0;
        keyFrame = false;
        buffer = nullptr;
        buffer_size = 0;
        capacity = 0;
    }

}VPacket;

class MediaClassEngine{
public:
    MediaClassEngine();
    ~MediaClassEngine();
    jobject createCodec(JNIEnv *env, int32_t width, int32_t height, bool useSurface);
    void startEncode(JNIEnv *env);
    void stopCodec(JNIEnv *env);
    int readPacket(JNIEnv *env, VPacket *packet);
    int writeFrame(JNIEnv *env, const uint8_t *buffer, const size_t size, int64_t pts);
private:
    void codecProcessThread();
    int createEnv(JNIEnv **env);
    void releaseEncode(JNIEnv *env);
private:
    int32_t mWidth;
    int32_t mHeight;
    std::shared_ptr<std::thread> m_codec_thread;
    std::atomic_bool m_bStop;
    ConfigData m_configure_data;

    bool m_use_Surface;
    jobject m_format;
    jobject m_android_media_codec;
    jobject  m_output_buffer_info;
    //jobjectArray m_input_buffer_array;

private:
    VPacket m_temp_packet;
    void reallocVPacket(size_t size){
        if(size <= 0)
            return;
        if(m_temp_packet.capacity < size ||
                m_temp_packet.capacity  > size + 1024 * 300){
            if(m_temp_packet.capacity > 0){
                free(m_temp_packet.buffer);
            }
            m_temp_packet.capacity = size;
            m_temp_packet.buffer = (uint8_t*)malloc(m_temp_packet.capacity);
        }
    }
    void freeVPacket(){
        if(m_temp_packet.capacity > 0){
            free(m_temp_packet.buffer);
        }
        m_temp_packet.initPacket();
    }
};

#endif