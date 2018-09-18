//
// Created by jiangjie on 2018/8/27.
//

#ifndef CORE_MEDIA_COMMON_MEDIA_STRUCT_H
#define CORE_MEDIA_COMMON_MEDIA_STRUCT_H

#include <string>
#include <inttypes.h>
#include "core/common.h"
__EXTERN_C_BEGIN
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
__EXTERN_C_END

namespace av{
    static void vp_frame_rescale_ts(AVFrame *frame, AVRational src_tb, AVRational dst_tb)
    {
        if (frame->pts != AV_NOPTS_VALUE)
            frame->pts = av_rescale_q(frame->pts, src_tb, dst_tb);
        if (frame->pkt_pts != AV_NOPTS_VALUE)
            frame->pkt_pts = av_rescale_q(frame->pkt_pts, src_tb, dst_tb);
        if (frame->pkt_dts != AV_NOPTS_VALUE)
            frame->pkt_dts = av_rescale_q(frame->pkt_dts, src_tb, dst_tb);
        if (frame->pkt_duration > 0)
            frame->pkt_duration = av_rescale_q(frame->pkt_duration, src_tb, dst_tb);
    }


    typedef  struct{
        std::string codec_name;
        int codec_type = -1;
        int codec_id = -1;

        uint8_t *extradata = nullptr;
        int extradata_size = 0;

        int width = 0;
        int height = 0;
        int frame_rate_num= 0;
        int frame_rate_den = 0;
    }VCodecParm;

    typedef  struct{
        std::string codec_name;
        int codec_type = -1;
        int codec_id = -1;

        int channels = 0;
        int sample_rate = 0;
        int sample_depth = 0;
        int frame_size = 0;
    }ACodecParm;

    typedef struct{
        long duration = 0;
        long start_time = 0;
        int bitrate = 0 ;

    }StreamInfo;

    struct AVCodecContextDeleter {
        void operator()(AVCodecContext* ptr) const {if(ptr)avcodec_free_context(&ptr); }
    };

    struct AVFormatContextInputDeleter{
        void operator()(AVFormatContext* ptr) const { if(ptr)avformat_close_input(&ptr); }
    };
    struct AVFormatContextOutputDeleter{
        void operator()(AVFormatContext* ptr) const {
            if(ptr){
                if (ptr->pb && !(ptr->oformat->flags & AVFMT_NOFILE))
                    avio_closep(&ptr->pb);
                avformat_close_input(&ptr);
            }
        }
    };

    struct AVFrameDeleter{
        void operator()(AVFrame* ptr) const {
            if(ptr){
                av_frame_free(&ptr);
            }
        }
    };

    struct AVPacketDeleter{
        void operator()(AVPacket* ptr) const {
            if(ptr){
                av_packet_free(&ptr);
            }
        }
    };

    struct AVBSFContextDeleter{
        void operator()(AVBSFContext* ptr) const {
            if(ptr){
                av_bsf_free(&ptr);
            }
        }
    };

    struct AVFilterGraphDeleter{
        void operator()(AVFilterGraph* ptr) const {
            if(ptr){
                avfilter_graph_free(&ptr);
            }
        }
    };

    struct AVFilterInOutDeleter{
        void operator()(AVFilterInOut* ptr) const {
            if(ptr){
                avfilter_inout_free(&ptr);
            }
        }
    }; 

    struct AVCodecParametersDeleter {
        void operator()(AVCodecParameters* ptr) const {
            if (ptr) {
                avcodec_parameters_free(&ptr);
            }
        }
    };
}

#endif //ANDROIDGRAFIKA_MEDIA_STRUCT_H
