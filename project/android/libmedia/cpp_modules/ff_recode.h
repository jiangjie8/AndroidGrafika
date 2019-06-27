#ifndef ANDROIDGRAFIKA_FF_RECODE_H
#define ANDROIDGRAFIKA_FF_RECODE_H


#include <thread>
#include <memory>
#include <functional>
#include "media_common/java_avstruct.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/av_decoder.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/av_encoder.h"
#include "core/media_common/av_demuxer.h"
#include "core/media_common/av_muxer.h"
#include "core/media_common/raw_stream_parser.h"
#include "core/media_common/bsf_filter.h"
#include "core/media_common/raw_filter.h"
namespace av {

class FFRecoder {
public:
    VCodecParm m_video_codecParam = VCodecParm();
    ACodecParm m_audio_codecParam = ACodecParm();
    StreamInfo m_stream_info = StreamInfo();

private:

    FilterGraph mFilterGraph;
    std::unique_ptr<AVMuxer> m_muxer = nullptr;
    std::unique_ptr<AVDecoder> m_audio_decoder = nullptr;
    std::unique_ptr<AVEncoder> m_audio_encoder = nullptr;
    std::unique_ptr<AVDemuxer> m_inputFormat = nullptr;
    std::unique_ptr<AVDemuxer> m_audioStream = nullptr;
    std::unique_ptr<AVBSFContext, AVBSFContextDeleter> m_mp4H264_bsf = nullptr;
    std::string m_input;
    int64_t m_timestamp_start = AV_NOPTS_VALUE;
    uint8_t *m_spspps_buffer = nullptr;
    int m_spspps_buffer_size = 0;
    int v_index = -1;
    int a_index = -1;
    bool m_video_eof{false};
    int64_t m_previous_apts{AV_NOPTS_VALUE};
    int64_t m_creation_time_us;
public:
    FFRecoder() = default;
    ~FFRecoder(){
        release();
    }
    int openInputFormat(JNIEnv *env, const char *inputStr);

    int closeInputFormat(){
        m_inputFormat.reset();
        m_audioStream.reset();
        return 0;
    }
    int getMediaInfo(JNIEnv *env, jobject mediaInfo);
    int openOutputFormat(JNIEnv *env, const char *outputStr, jobject mediaInfo);
    //delete this object after closeOutputFormat
    int closeOutputFormat();

    int readVideoPacket(JNIEnv *env, jobject packet_obj);

    int writePacket(JNIEnv *env, jobject packet_obj);

private:
    int initH264Bsf(const AVCodecParameters *codecpar);

    int writeAudioPacket(int64_t video_pts);
    int getAudioFrame(AVFrame *frame);
    int filterAudioFrame(AVFrame *frame);
    int drainAudioEncoder(const AVFrame *frame, AVPacket *packet);

    int64_t parser_creation_time();
    void set_create_timestamp(int64_t time_second);

    int writeInterleavedPacket(AVPacket *packet);

    void release(){
        if(m_spspps_buffer != nullptr){
            av_free(m_spspps_buffer);
            m_spspps_buffer = nullptr;
            m_spspps_buffer_size = 0;
        }
        m_inputFormat.reset();
        m_audioStream.reset();
        m_muxer.reset();
        m_audio_decoder.reset();
        m_audio_encoder.reset();
        m_mp4H264_bsf.reset();
    }

};


}

#endif