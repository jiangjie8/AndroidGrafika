//
// Created by jiangjie on 2018/8/27.
//

#ifndef ANDROIDGRAFIKA_AV_MUXER_H
#define ANDROIDGRAFIKA_AV_MUXER_H
#include <thread>
#include <memory>
#include <functional>
#include "core/media_common/av_decoder.h"
#include "core/media_common/media_struct.h"
#include "core/media_common/av_encoder.h"

namespace av {
class AVMuxer {
public:

    AVMuxer() = default;
    ~AVMuxer() = default;

    int openOutputFormat(const char *outputStr);

    /*
     * return stream index;
     * */
    int addStream(AVCodecParameters *parm);
    int setExternalData(uint8_t *data, size_t size, int stream_index);
    int writeHeader();
    int writePacket(AVPacket *packet);

    void closeOutputForamt(){
        if(m_outputFormat)
            av_write_trailer(m_outputFormat.get());
        m_outputFormat.reset();
    }

public:
    std::unique_ptr<AVFormatContext, AVFormatContextOutputDeleter> m_outputFormat = nullptr;
    std::string m_output;
};
}

#endif //ANDROIDGRAFIKA_AV_MUXER_H
