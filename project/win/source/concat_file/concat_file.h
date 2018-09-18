#pragma once

#include "core/media_common/media_struct.h"
#include "core/media_common/bsf_filter.h"
#include "core/media_common/raw_stream_parser.h"
#include "core/media_common/av_demuxer.h"
#include "core/media_common/av_muxer.h"
#include "core/media_common/av_decoder.h"
#include "core/media_common/av_encoder.h"
#include <iostream>
#include <functional>
#include <iostream>
#include <thread>
#include <windows.h>
#include <string>
#include <type_traits>
#include <thread>
#include <mutex>
#include <memory>
#include <map>
#include <deque>

namespace av {

    constexpr const char *VP_SMALL = "vp_small";
    constexpr const char *VP_LARGER = "vp_larger";

    enum class VStreamType : uint8_t
    {
        UNKnow,
        SMALL,
        LARGE
    };

    typedef struct VideoPadding {
        VStreamType streamType = VStreamType::UNKnow;
        int64_t start_pts = AV_NOPTS_VALUE;
        int64_t end_pts = AV_NOPTS_VALUE;
        VideoPadding(VStreamType type, int64_t startpts, int64_t endpts) :
            streamType(type), start_pts(startpts), end_pts(endpts)
        {
        }
    }VideoPadding;


class MergerCtx {

public:
    MergerCtx() = default;
    ~MergerCtx() {
        m_aStream1.reset();
        m_vStream1.reset();
        m_vStream2.reset();
        m_decodeV1.reset();
        m_decodeV2.reset();
        m_output.reset();
        m_encodeV.reset();
    }
    //void probeSEI();
    int openInputStreams(const char *file1, const char *file2);

    int openOutputFile(const char *file);

    int cfgCodec();

    int mergerLoop();
    
private:
    std::unique_ptr<AVDemuxer> m_aStream1 = nullptr;
    std::unique_ptr<AVDemuxer> m_vStream1 = nullptr;
    std::unique_ptr<AVDemuxer> m_vStream2 = nullptr;
    std::unique_ptr<AVDecoder> m_decodeV1 = nullptr;
    std::unique_ptr<AVDecoder> m_decodeV2 = nullptr;
    std::unique_ptr<AVMuxer> m_output = nullptr;
    std::unique_ptr<AVEncoder> m_encodeV = nullptr;

    std::map<int64_t, VideoPadding> m_sei_info1;
    std::map<int64_t, VideoPadding> m_sei_info2;

    AVRational m_frame_rate;
    int m_videIndex = -1;
    int m_audioIndex = -1;

    int getVideoFrame(AVDemuxer *demuxer, AVDecoder *decoder, AVFrame *frame);
    int writeAudioPacket(AVDemuxer *input, AVMuxer  *output, int64_t pts_flag, int index);
    int encodeWriteFrame(AVEncoder *encoder, AVFrame *frame);

};

}