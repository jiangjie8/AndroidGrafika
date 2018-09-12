//
// Created by jiangjie on 2018/8/27.
//

#include "av_muxer.h"
namespace av {

    int AVMuxer::openOutputFormat(const char *outputStr) {
        AVFormatContext *ofmt_ctx = nullptr;
        m_output = outputStr;
        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outputStr);
        if(ofmt_ctx == nullptr){
            return -1;
        }
        m_outputFormat.reset(ofmt_ctx);
        return 0;
    }

    int AVMuxer::addStream(AVCodecParameters *parm){
        int ret = 0;
        AVFormatContext *ofmt_ctx = m_outputFormat.get();
        AVStream *stream = avformat_new_stream(ofmt_ctx, nullptr);
        if(stream == nullptr){
            LOGE("avformat_new_stream error");
            return -1;
        }
        avcodec_parameters_copy(stream->codecpar, parm);
        stream->codecpar->codec_tag = 0;
        avcodec_parameters_to_context(stream->codec, stream->codecpar);
        return (ofmt_ctx->nb_streams-1);
    }

    int AVMuxer::setExternalData(uint8_t *data, size_t size, int stream_index){
        AVStream *stream = m_outputFormat->streams[stream_index];
        if(stream->codecpar->extradata == nullptr && data != nullptr){
            stream->codecpar->extradata_size = size;
            stream->codecpar->extradata = (uint8_t *)av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(stream->codecpar->extradata, data, size);
        }
        return 0;
    }


    int AVMuxer::writeHeader(){
        int ret = 0;
        AVFormatContext *ofmt_ctx = m_outputFormat.get();
        av_dump_format(ofmt_ctx, 0, m_output.c_str(), 1);
        if (!(ofmt_ctx->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ofmt_ctx->pb, m_output.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                LOGE("Could not open output file '%s'", m_output.c_str());
                return -1;
            }
        }
        ret = avformat_write_header(ofmt_ctx, NULL);
        if (ret < 0) {
            LOGE("Error occurred when avformat_write_header");
            return ret;
        }
        return ret;
    }


    int AVMuxer::writePacket(AVPacket *packet){
        int ret = 0;
        AVFormatContext *ofmt_ctx = m_outputFormat.get();
        int stream_index = packet->stream_index;
        AVRational stream_timebae = ofmt_ctx->streams[stream_index]->time_base;
//        ALOGE("index %d pst   %lld  size %d  flags %d ",packet->stream_index, packet->pts, packet->size, packet->flags);
        av_packet_rescale_ts(packet, { 1, AV_TIME_BASE }, stream_timebae);
        ret = av_interleaved_write_frame(m_outputFormat.get(), packet);
        if (ret < 0) {
            LOGE("write packet error");
            return ret;
        }
        return ret;
    }
}