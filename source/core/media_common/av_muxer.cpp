//
// Created by jiangjie on 2018/8/27.
//

#include "core/media_common/av_muxer.h"
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

    int AVMuxer::addStream(const AVCodecParameters *parm){
        if (parm == nullptr)
            return -1;
        int ret = 0;
        AVStream *stream = avformat_new_stream(m_outputFormat.get(), nullptr);
        if(stream == nullptr){
            LOGE("avformat_new_stream error\n");
            return -1;
        }
        avcodec_parameters_copy(stream->codecpar, parm);
        avcodec_parameters_to_context(stream->codec, stream->codecpar);
        return (m_outputFormat.get()->nb_streams-1);
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

    int AVMuxer::setMetaDate(const char *key, const char *value) {
        return av_dict_set(&m_outputFormat->metadata, key, value, 0);
    }

    int AVMuxer::writeHeader(){
        int ret = 0;
        AVFormatContext *ofmt_ctx = m_outputFormat.get();
        if (!(ofmt_ctx->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ofmt_ctx->pb, m_output.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                LOGE("Could not open output file '%s'  ret %d\n", m_output.c_str(), ret);
                return ret;
            }
        }
        ret = avformat_write_header(ofmt_ctx, NULL);
        av_dump_format(ofmt_ctx, 0, m_output.c_str(), 1);
        if (ret < 0) {
            LOGE("Error occurred when avformat_write_header\n");
            return ret;
        }
        return ret;
    }


    int AVMuxer::writePacket(AVPacket *packet){
        int ret = 0;
        int stream_index = packet->stream_index;
        AVRational stream_timebae = m_outputFormat->streams[stream_index]->time_base;
        av_packet_rescale_ts(packet, { 1, AV_TIME_BASE }, stream_timebae);
        ret = av_interleaved_write_frame(m_outputFormat.get(), packet);
        if (ret < 0) {
            LOGE("write packet error\n");
            return ret;
        }
        return ret;
    }
}