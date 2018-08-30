#include "audio_decoder.h"

namespace av{

    int AudioDecoder::openCoder(const AVCodecParameters *parm){
        int ret;
        const AVCodec *decoder = avcodec_find_decoder(parm->codec_id);

        m_codec_ctx.reset(avcodec_alloc_context3(decoder));
        if(m_codec_ctx == nullptr){
            return -1;
        }
        avcodec_parameters_to_context(m_codec_ctx.get(), parm);
        ret = avcodec_open2(m_codec_ctx.get(), decoder, nullptr);
        if(ret < 0){
            ALOGE("Failed to open decoder  %d", parm->codec_id);
            m_codec_ctx.reset();
            return -1;
        }
        return 0;
    }


    int AudioDecoder::sendPacket(const AVPacket *packet){
        int ret = 0;
        ret = avcodec_send_packet(m_codec_ctx.get(), packet);
        return ret;
    }

    int AudioDecoder::receiveFrame(AVFrame *frame){
        int ret = 0;
        ret = avcodec_receive_frame(m_codec_ctx.get(), frame);
        return ret;
    }


    int AudioDecoder::flushCodec(AVFrame *frame){
        int ret = 0;
        if(!m_inputPacketEnd){
            ret = avcodec_send_packet(m_codec_ctx.get(), nullptr);
            if(ret == AVERROR(EAGAIN)){
                ret = 0;
            }
            else if(ret == 0){
                m_inputPacketEnd = true;
            }
            else{

            }
        }
        if(ret < 0)
            return ret;

        ret = avcodec_receive_frame(m_codec_ctx.get(), frame);
        return ret;
    }

}