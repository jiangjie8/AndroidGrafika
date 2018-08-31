#ifndef ANDROIDGRAFIKA_AUDIO_ENCODER_H
#define ANDROIDGRAFIKA_AUDIO_ENCODER_H

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
class AudioEncoder{
private:
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> m_codec_ctx = nullptr;
    bool  m_inputFrameEnd = false;

public:

    AudioEncoder() = default;
    ~AudioEncoder() = default;
    int openCoder(const AVCodecParameters *parm);
    int sendFrame(const AVFrame *frame);
    int receivPacket(AVPacket *avpkt);
    int flushCodec(AVPacket *avpkt);
    void closeCoder(){
        m_codec_ctx.reset();
    }

    const AVCodecContext *getCodecContext(){
        return m_codec_ctx.get();
    }
private:
    //    int encode_audio(AVPacket *packet, const AVFrame *frame);
};
}
#endif  // MODULES_AUDIO_CODING_CODECS_AUDIO_ENCODER_H_
