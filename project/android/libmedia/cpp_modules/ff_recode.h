#ifndef ANDROIDGRAFIKA_FF_RECODE_H
#define ANDROIDGRAFIKA_FF_RECODE_H


#include <thread>
#include <memory>
#include <functional>
#include "media_common/java_avstruct.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/audio_decoder.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/audio_encoder.h"
#include "core/media_common/av_demuxer.h"
#include "core/media_common/av_muxer.h"
#include "core/media_common/raw_stream_parser.h"
#include "core/media_common/bsf_filter.h"
namespace av {

class FFRecoder {
public:
    VCodecParm m_video_codecParam = VCodecParm();
    ACodecParm m_audio_codecParam = ACodecParm();
    StreamInfo m_stream_info = StreamInfo();

private:

    AVFilterContext *m_buffersink_ctx = nullptr;
    AVFilterContext *m_buffersrc_ctx = nullptr;
    std::unique_ptr<AVFilterGraph, AVFilterGraphDeleter> m_pcm_filter = nullptr;

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
    int getMediaInfo(JNIEnv *env, jobject object);
    int openOutputFormat(JNIEnv *env, const char *outputStr, int codecID);
    //delete this object after closeOutputFormat
    int closeOutputFormat();

    int readPacket(JNIEnv *env, jobject packet_obj);
    int writePacket(JNIEnv *env, jobject packet_obj);

private:
    int init_filter(const AVCodecContext *dec_ctx, const AVCodecContext *enc_ctx, const char *filter_spec);
    int initH264Bsf(const AVCodecParameters *codecpar);
    int recodeWriteAudio(AVPacket *packet);
    int drainAudioEncoder(const AVFrame *frame);
    int probeMedia();
    const AVCodecParameters *getCodecParameters(enum AVMediaType type);
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