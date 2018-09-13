#include "av_encoder.h"
namespace av{


    int AVEncoder::cfgCodec(const AVCodecParameters *parm){
        int ret;
        enum AVCodecID codec_id = parm->codec_id;
        m_codec = avcodec_find_encoder(codec_id);
        m_codec_ctx.reset(avcodec_alloc_context3(m_codec));
        if(m_codec_ctx == nullptr){
            return -1;
        }
        
        if (parm->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_codec_ctx->bit_rate = parm->bit_rate;
            m_codec_ctx->sample_rate = parm->sample_rate;
            m_codec_ctx->channels = parm->channels;
            m_codec_ctx->channel_layout = av_get_default_channel_layout(m_codec_ctx->channels);
            m_codec_ctx->sample_fmt = m_codec->sample_fmts[0];
            m_codec_ctx->time_base = { 1, m_codec_ctx->sample_rate };
        }
        else if (parm->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_codec_ctx->height = parm->height;
            m_codec_ctx->width = parm->width;
            m_codec_ctx->sample_aspect_ratio = parm->sample_aspect_ratio;
            /* take first format from list of supported formats */
            if (m_codec->pix_fmts)
                m_codec_ctx->pix_fmt = m_codec->pix_fmts[0];
            else
                m_codec_ctx->pix_fmt = (enum AVPixelFormat)parm->format;
            /* video time_base can be set to whatever is handy and supported by encoder */
            m_codec_ctx->time_base = { 1, 150 };
        }
        return 0;
    }

    int AVEncoder::openCodec() {
        if (m_codec_ctx == nullptr)
            return -1;
        int ret = 0;
        ret = avcodec_open2(m_codec_ctx.get(), m_codec, nullptr);
        if (ret < 0) {
            LOGE("Failed to open encoder ");
            m_codec_ctx.reset();
            return -1;
        }
        return ret;
    }

    int AVEncoder::sendFrame(const AVFrame *frame){
        int ret = 0;
        ret = avcodec_send_frame(m_codec_ctx.get(), frame);
        return ret;
    }

    int AVEncoder::receivPacket(AVPacket *avpkt){
        int ret = 0;
        ret = avcodec_receive_packet(m_codec_ctx.get(), avpkt);
        return ret;
    }

    int AVEncoder::flushCodec(AVPacket *avpkt){
        int ret = 0;
        while ((ret = avcodec_receive_packet(m_codec_ctx.get(), avpkt)) == AVERROR(EAGAIN)) {
            ret = avcodec_send_frame(m_codec_ctx.get(), nullptr);
            if (ret < 0) {
                LOGE("avcodec_send_frame error %d\n", ret);
                break;
            }
        }
        return ret;
    }

}