//
// Created by jiangjie on 2018/8/27.
//

#ifndef CORE_MEDIA_COMMON_AV_MUXER_H
#define CORE_MEDIA_COMMON_AV_MUXER_H
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
    int addStream(const AVCodecParameters *parm);
    int setExternalData(uint8_t *data, size_t size, int stream_index);
    int writeHeader();
    int writePacket(AVPacket *packet);

    int closeOutputForamt(){
        int ret = 0;
        if(m_outputFormat)
            ret = av_write_trailer(m_outputFormat.get());
        m_outputFormat.reset();
        return ret;
    }
    const AVOutputFormat *getOutputFormat() {
        return m_outputFormat->oformat;
    }
private:
    std::unique_ptr<AVFormatContext, AVFormatContextOutputDeleter> m_outputFormat = nullptr;
    std::string m_output;
};
}

#endif //ANDROIDGRAFIKA_AV_MUXER_H
