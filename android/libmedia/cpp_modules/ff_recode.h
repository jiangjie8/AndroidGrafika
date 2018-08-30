#ifndef ANDROIDGRAFIKA_FF_RECODE_H
#define ANDROIDGRAFIKA_FF_RECODE_H


#include <thread>
#include <memory>
#include <functional>
#include <media_common/media_struct.h>
#include "media_common/java_avstruct.h"
#include "media_common/audio_decoder.h"
#include "media_common/media_struct.h"
#include "media_common/audio_encoder.h"
#include "media_common/av_demuxer.h"
#include "media_common/av_muxer.h"
#include "media_common/raw_stream_parser.h"
namespace av {

class FFRecoder {
public:
    VCodecParm m_video_codecParam = VCodecParm();
    ACodecParm m_audio_codecParam = ACodecParm();
    StreamInfo m_stream_info = StreamInfo();

private:
    std::unique_ptr<AVMuxer> m_muxer = nullptr;
    std::unique_ptr<AudioDecoder> m_audio_decoder = nullptr;
    std::unique_ptr<AudioEncoder> m_audio_encoder = nullptr;
    std::unique_ptr<AVBSFContext, AVBSFContextDeleter> m_mp4H264_bsf = nullptr;
    std::unique_ptr<AVFormatContext, AVFormatContextInputDeleter> m_inputFormat = nullptr;

    std::string m_input;
    bool m_get_spspps = false;
    uint8_t *m_spspps_buffer = nullptr;
    int m_spspps_buffer_size = 0;
    int v_index = -1;
    int a_index = -1;
public:
    FFRecoder() = default;
    ~FFRecoder(){
        release();
    }
    int openInputFormat(JNIEnv *env, const char *inputStr);

    int closeInputFormat(){
        m_inputFormat.reset();
        return 0;
    }
    int probeMediaInfo(JNIEnv *env, jobject object);
    int openOutputFormat(JNIEnv *env, const char *outputStr);
    //delete this object after closeOutputFormat
    int closeOutputFormat(){
        m_muxer->closeOutputForamt();
        m_muxer.reset();
        return 0;
    }

    int readPacket(JNIEnv *env, jobject packet_obj);
    int writePacket(JNIEnv *env, jobject packet_obj);

private:
    int initH264Bsf(const AVCodecParameters *codecpar);
    int recodeWriteAudio(AVPacket *packet);
    int drainAudioEncoder(const AVFrame *frame);
    int getMediaInfo();
    const AVCodecParameters *getCodecParameters(enum AVMediaType type);
    int applyBitstream(AVPacket *packet);
    void release(){
        if(m_spspps_buffer != nullptr){
            av_free(m_spspps_buffer);
            m_spspps_buffer = nullptr;
            m_spspps_buffer_size = 0;
            m_get_spspps = false;
        }
        m_inputFormat.reset();
        m_muxer.reset();
        m_audio_decoder.reset();
        m_audio_encoder.reset();
        m_mp4H264_bsf.reset();
    }

};


}

#endif