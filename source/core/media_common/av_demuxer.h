#ifndef ANDROIDGRAFIKA_AV_DEMUXER_H
#define ANDROIDGRAFIKA_AV_DEMUXER_H


#include "core/media_common/media_struct.h"
#include "core/media_common/audio_decoder.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/audio_encoder.h"
#include <thread>
#include <memory>
#include <functional>

namespace av {

class AVDemuxer {

public:
    VCodecParm m_video_codecParam = VCodecParm();
    ACodecParm m_audio_codecParam = ACodecParm();
    StreamInfo m_stream_info = StreamInfo();

public:
    AVDemuxer() = default;

    ~AVDemuxer(){
        m_inputFormat.reset();
    }

    int openInputFormat(const char *inputStr);

    const AVCodecParameters *getCodecParameters(enum AVMediaType type);

    int readPacket(AVPacket *packet);

    void closeInputFormat() {
        m_inputFormat.reset();
    }

private:
    int getMediaInfo();

private:
    std::unique_ptr<AVFormatContext, AVFormatContextInputDeleter> m_inputFormat = nullptr;
    std::string m_input;

};


}

#endif