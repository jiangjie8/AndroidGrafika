#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include "media_struct.h"
__EXTERN_C_BEGIN
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
__EXTERN_C_END


#include <memory>
#include <functional>
namespace av{
class AVEncoder{
private:
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_codec_ctx = nullptr;
    AVCodec *m_codec = nullptr;
public:

    AVEncoder() = default;
    ~AVEncoder() = default;
    int cfgCodec(const AVCodecParameters *parm);
    int openCodec();
    
    int sendFrame(const AVFrame *frame);
    int receivPacket(AVPacket *avpkt);
    int flushCodec(AVPacket *avpkt);
    void closeCoder(){
        m_codec_ctx.reset();
    }

    AVCodecContext *getCodecContext(){
        return m_codec_ctx.get();
    }
};
}
#endif  // MODULES_AUDIO_CODING_CODECS_AUDIO_ENCODER_H_
