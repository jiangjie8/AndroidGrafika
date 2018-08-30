#include "audio_encoder.h"
namespace av{

    int AudioEncoder::openCoder(const AVCodecParameters *parm){
        int ret;
        enum AVCodecID codec_id = AV_CODEC_ID_AAC;
        const AVCodec *encoder = avcodec_find_encoder(codec_id);

        m_codec_ctx.reset(avcodec_alloc_context3(encoder));
        if(m_codec_ctx == nullptr){
            return -1;
        }
        AVCodecContext *codec_ctx = m_codec_ctx.get();

//        avcodec_parameters_to_context(m_codec_ctx.get(), parm);
        codec_ctx->sample_rate = parm->sample_rate;
        codec_ctx->channels = parm->channels;
        codec_ctx->channel_layout = av_get_default_channel_layout(codec_ctx->channels);
        codec_ctx->sample_fmt = encoder->sample_fmts[0];
        codec_ctx->time_base = {1, codec_ctx->sample_rate};
        ret = avcodec_open2(m_codec_ctx.get(), encoder, nullptr);
        if(ret < 0){
            ALOGE("Failed to open decoder  %d", parm->codec_id);
            m_codec_ctx.reset();
            return -1;
        }

        return 0;
    }

    int AudioEncoder::sendFrame(const AVFrame *frame){
        int ret = 0;
        ret = avcodec_send_frame(m_codec_ctx.get(), frame);
        return ret;
    }

    int AudioEncoder::receivPacket(AVPacket *avpkt){
        int ret = 0;
        ret = avcodec_receive_packet(m_codec_ctx.get(), avpkt);
        return ret;
    }


    int AudioEncoder::encode_audio(AVPacket *packet, const AVFrame *frame){
        int ret = 0;
        int gotpacket = 0;
        ret = avcodec_encode_audio2(m_codec_ctx.get(), packet, frame, &gotpacket);
        if(ret < 0){
            ALOGE("avcodec_encode_audio2 error");
            return ret;
        }
        else if(gotpacket)
            return 0;
        else
            return AVERROR(EAGAIN);
    }

    int AudioEncoder::flushCodec(AVPacket *avpkt){
        int ret = 0;
        if(!m_inputFrameEnd){
            ret = avcodec_send_frame(m_codec_ctx.get(), nullptr);
            if(ret == AVERROR(EAGAIN)){
                ret = 0;
            }
            else if(ret == 0){
                m_inputFrameEnd = true;
            }
            else{

            }
        }

        if(ret < 0)
            return ret;

        ret = avcodec_receive_packet(m_codec_ctx.get(), avpkt);
        return ret;

    }

}