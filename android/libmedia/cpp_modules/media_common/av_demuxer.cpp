#include "av_demuxer.h"
namespace av{


    int AVDemuxer::openInputFormat(const char *inputStr) {
        m_input = inputStr;
        AVFormatContext *ifmt_ctx = nullptr;
        int ret = 0;

        ret = avformat_open_input(&ifmt_ctx, inputStr, nullptr, nullptr);
        if (ret < 0) {
            ALOGE("Could not open input file '%s'   %d", inputStr, ret);
            return ret;
        }

        if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
            avformat_close_input(&ifmt_ctx);
            ALOGE("Failed to retrieve input stream information");
            return ret;
        }
        getMediaInfo();
        m_inputFormat.reset(ifmt_ctx);
        ifmt_ctx = nullptr;
        return ret;
    }

    const AVCodecParameters* AVDemuxer::getCodecParameters(enum AVMediaType type){
        AVFormatContext *ifmt_ctx = m_inputFormat.get();
        m_stream_info.duration = ifmt_ctx->duration;
        m_stream_info.bitrate = ifmt_ctx->bit_rate;
        for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
            AVStream *stream = ifmt_ctx->streams[i];
            AVMediaType mediaType = stream->codecpar->codec_type;
            if(mediaType == type){
                return stream->codecpar;
            }
        }
        return nullptr;
    }

    int AVDemuxer::readPacket(AVPacket *packet) {

        int ret = av_read_frame(m_inputFormat.get(), packet);
        return ret;
    }


    int AVDemuxer::getMediaInfo() {
        if (m_inputFormat == nullptr)
            return -1;
        AVFormatContext *ifmt_ctx = m_inputFormat.get();

        for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
            AVStream *stream = ifmt_ctx->streams[i];
            AVMediaType mediaType = stream->codecpar->codec_type;
            if (mediaType == AVMEDIA_TYPE_VIDEO) {
                m_video_codecParam.codec_id = stream->codecpar->codec_id;
                m_video_codecParam.width = stream->codecpar->width;
                m_video_codecParam.height = stream->codecpar->height;
                m_video_codecParam.frame_rate = av_q2d(stream->avg_frame_rate);

            } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
                m_audio_codecParam.codec_id = stream->codecpar->codec_id;
                m_audio_codecParam.channels = stream->codecpar->channels;
                m_audio_codecParam.sample_rate = stream->codecpar->sample_rate;
                m_audio_codecParam.sample_depth = stream->codecpar->bits_per_coded_sample;
                m_audio_codecParam.frame_size = stream->codecpar->frame_size;
            }
        }
        return 0;
    }
}

