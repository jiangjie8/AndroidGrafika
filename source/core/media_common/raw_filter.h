#ifndef CORE_MEDIA_COMMON_RAW_FILTER_H
#define CORE_MEDIA_COMMON_RAW_FILTER_H

#include "core/media_common/media_struct.h"
#include <thread>
#include <memory>
namespace av {
class FilterGraph {
public:
    FilterGraph() = default;
    ~FilterGraph(){
        m_graph_filter = nullptr;
        m_buffersink_ctx = nullptr;
        m_buffersrc_ctx = nullptr;
    }
    int initFilter(const AVCodecContext *dec_ctx, const AVCodecContext *enc_ctx, const char *filter_spec);
    int addFrame(AVFrame *frame) {
        if (m_graph_filter == nullptr) {
            return -1;
        }
        int ret = 0;
        ret = av_buffersrc_add_frame_flags(m_buffersrc_ctx, frame, 0);
        if (ret < 0) {
            LOGE("Error while feeding the filtergraph\n");
        }
        return ret;
    }
    int getFrame(AVFrame *frame) {
        if (m_graph_filter == nullptr) {
            return -1;
        }
        return av_buffersink_get_frame(m_buffersink_ctx, frame);;
    }
private:
    std::unique_ptr<AVFilterGraph, AVFilterGraphDeleter> m_graph_filter = nullptr;
    AVFilterContext *m_buffersink_ctx = nullptr;
    AVFilterContext *m_buffersrc_ctx = nullptr;


};

}


#endif