#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "core/media_common/media_struct.h"
#include "core/common.h"
__EXTERN_C_BEGIN
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
__EXTERN_C_END


#include <memory>
#include <functional>


namespace av{

class AVDecoder{
private:
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_codec_ctx = nullptr;
    AVCodec *m_codec = nullptr;

public:

    AVDecoder() = default;
    ~AVDecoder() = default;

    int cfgCodec(const AVCodecParameters *parm);
    int openCodec();
    int sendPacket(const AVPacket *packet);
    int receiveFrame(AVFrame *frame);
    int flushCodec(AVFrame *frame);
    void closeCoder(){
        m_codec_ctx.reset();
    }

    AVCodecContext *getCodecContext(){
        return m_codec_ctx.get();
    }

};

}



#endif  // MODULES_AUDIO_CODING_CODECS_AUDIO_DECODER_H_
