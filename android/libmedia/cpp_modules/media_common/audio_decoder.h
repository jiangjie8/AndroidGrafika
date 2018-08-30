#ifndef ANDROIDGRAFIKA_AUDIO_DECODER_H
#define ANDROIDGRAFIKA_AUDIO_DECODER_H

#include "jni_bridge.h"
__EXTERN_C_BEGIN
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
__EXTERN_C_END

#include "media_struct.h"
#include <memory>
#include <functional>


namespace av{



class AudioDecoder{
private:
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_codec_ctx = nullptr;
    bool  m_inputPacketEnd = false;
public:

    AudioDecoder() = default;
    ~AudioDecoder() = default;

    int openCoder(const AVCodecParameters *parm);

    int sendPacket(const AVPacket *packet);
    int receiveFrame(AVFrame *frame);

    int flushCodec(AVFrame *frame);


    void closeCoder(){
        m_codec_ctx.reset();
    }

    const AVCodecContext *getCodecContext(){
        return m_codec_ctx.get();
    }


};

}



#endif  // MODULES_AUDIO_CODING_CODECS_AUDIO_DECODER_H_
