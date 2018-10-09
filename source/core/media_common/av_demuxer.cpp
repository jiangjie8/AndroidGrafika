#include "core/media_common/av_demuxer.h"
namespace av{
    int AVDemuxer::openInputFormat(const char *inputStr) {
        int ret = 0;
        m_input = inputStr;
        AVFormatContext *ifmt_ctx = nullptr;
        ret = avformat_open_input(&ifmt_ctx, inputStr, nullptr, nullptr);
        if (ret < 0) {
            LOGE("Could not open input file '%s'   %d", inputStr, ret);
            return ret;
        }

        if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
            avformat_close_input(&ifmt_ctx);
            LOGE("Failed to retrieve input stream information");
            return ret;
        }
        m_inputFormat.reset(ifmt_ctx); ifmt_ctx = nullptr;
        getMediaInfo();
        
        //av_dump_format(m_inputFormat.get(), -1, inputStr, 0);
        return ret;
    }

    const AVCodecParameters* AVDemuxer::getCodecParameters(enum AVMediaType type){
        if (m_inputFormat == nullptr)
            return nullptr;

        for (int i = 0; i < m_inputFormat->nb_streams; i++) {
            AVStream *stream = m_inputFormat->streams[i];
            AVMediaType mediaType = stream->codecpar->codec_type;
            if(mediaType == type){
                return stream->codecpar;
            }
        }
        return nullptr;
    }

    int AVDemuxer::readPacket(AVPacket *packet) {
        if (m_readEOF) {
            return AVERROR_EOF;
        }
        int ret = 0;
        if (packet->data != nullptr) {
            av_packet_unref(packet);
        }
        ret = av_read_frame(m_inputFormat.get(), packet);
        if (ret == AVERROR_EOF) {
            m_readEOF = true;
        }
        else if (ret == 0) {
            int stream_index = packet->stream_index;
            AVRational stream_timebae = m_inputFormat->streams[stream_index]->time_base;
            av_packet_rescale_ts(packet, stream_timebae, { 1, AV_TIME_BASE });
        }
        return ret;
    }


    int AVDemuxer::getMediaInfo() {
        if (m_inputFormat == nullptr)
            return -1;
        stream_info.duration = m_inputFormat->duration;
        stream_info.bitrate = m_inputFormat->bit_rate;
        for (int i = 0; i < m_inputFormat->nb_streams; i++) {
            AVStream *stream = m_inputFormat->streams[i];
            AVMediaType mediaType = stream->codecpar->codec_type;
            if (mediaType == AVMEDIA_TYPE_VIDEO) {
                video_codecParam.codec_type = AVMEDIA_TYPE_VIDEO;
                video_codecParam.codec_id = stream->codecpar->codec_id;
                video_codecParam.width = stream->codecpar->width;
                video_codecParam.height = stream->codecpar->height;
                auto framerate = av_guess_frame_rate(m_inputFormat.get(), stream, nullptr);
                video_codecParam.frame_rate_num = framerate.num;
                video_codecParam.frame_rate_den = framerate.den;

            } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
                audio_codecParam.codec_type = AVMEDIA_TYPE_AUDIO;
                audio_codecParam.codec_id = stream->codecpar->codec_id;
                audio_codecParam.channels = stream->codecpar->channels;
                audio_codecParam.sample_rate = stream->codecpar->sample_rate;
                audio_codecParam.sample_depth = stream->codecpar->bits_per_coded_sample;
                audio_codecParam.frame_size = stream->codecpar->frame_size;
            }
        }
        return 0;
    }
}

