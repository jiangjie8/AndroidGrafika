#ifndef CORE_MEDIA_COMMON_AV_DEMUXER_H
#define CORE_MEDIA_COMMON_AV_DEMUXER_H


#include "core/media_common/media_struct.h"
#include "core/media_common/av_decoder.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/av_encoder.h"
#include <thread>
#include <memory>
#include <functional>

namespace av {

class AVDemuxer {

public:
    VCodecParm video_codecParam = VCodecParm();
    ACodecParm audio_codecParam = ACodecParm();
    StreamInfo stream_info = StreamInfo();

public:
    AVDemuxer() = default;

    ~AVDemuxer(){
        m_inputFormat.reset();
    }

    int openInputFormat(const char *inputStr);

    const AVCodecParameters *getCodecParameters(enum AVMediaType type);

    const AVInputFormat *getInputFormat() {
        return m_inputFormat->iformat;
    }

    const AVStream *getStream(enum AVMediaType type) {
        for (int i = 0; i < m_inputFormat->nb_streams; ++i) {
            AVStream *stream = m_inputFormat->streams[i];
            if (stream->codecpar->codec_type == type) {
                return stream;
            }
        }
        return nullptr;
    }
    const AVStream *getStream(int index) {
        if(index < m_inputFormat->nb_streams)
            return m_inputFormat->streams[index];
        return nullptr;
    }
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