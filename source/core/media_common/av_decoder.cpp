#include "av_decoder.h"

namespace av{

    int AVDecoder::cfgCodec(const AVCodecParameters *parm){
        int ret;
        m_codec = avcodec_find_decoder(parm->codec_id);
        m_codec_ctx.reset(avcodec_alloc_context3(m_codec));
        if(m_codec_ctx == nullptr){
            return -1;
        }
        avcodec_parameters_to_context(m_codec_ctx.get(), parm);
        if (parm->codec_type == AVMEDIA_TYPE_AUDIO && !m_codec_ctx->channel_layout)
            m_codec_ctx->channel_layout = av_get_default_channel_layout(m_codec_ctx->channels);

        return 0;
    }

    int AVDecoder::openCodec() {
        if (m_codec_ctx == nullptr)
            return -1;
        int ret = 0;
        ret = avcodec_open2(m_codec_ctx.get(), m_codec, nullptr);
        if (ret < 0) {
            LOGE("Failed to open decoder \n");
            m_codec_ctx.reset();
            return -1;
        }
        return ret;
    }


    int AVDecoder::sendPacket(const AVPacket *packet){
        int ret = 0;
        ret = avcodec_send_packet(m_codec_ctx.get(), packet);
        return ret;
    }

    int AVDecoder::receiveFrame(AVFrame *frame){
        int ret = 0;
        ret = avcodec_receive_frame(m_codec_ctx.get(), frame);
        return ret;
    }


    int AVDecoder::flushCodec(AVFrame *frame){
        int ret = 0;
        while ((ret = avcodec_receive_frame(m_codec_ctx.get(), frame)) == AVERROR(EAGAIN)) {
            ret = avcodec_send_packet(m_codec_ctx.get(), nullptr);
            if (ret < 0) {
                LOGE("avcodec_send_packet error %d\n", ret);
                break;
            }
        }
        return ret;
    }

}