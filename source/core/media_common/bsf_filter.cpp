#include "core/media_common/bsf_filter.h"



AVBSFContext* initBsfFilter(const AVCodecParameters *codecpar, const char *filter_name) {

    const AVBitStreamFilter * bsf = av_bsf_get_by_name(filter_name);
    if (bsf == nullptr) {
        LOGE(" find  %s  filter error", filter_name);
        return nullptr;
    }
    AVBSFContext *bsf_ctx = nullptr;
    av_bsf_alloc(bsf, &bsf_ctx);
    if (bsf_ctx == nullptr) {
        LOGE("av_bsf_alloc error");
        return nullptr;
    }
    avcodec_parameters_copy(bsf_ctx->par_in, codecpar);
    if (av_bsf_init(bsf_ctx) < 0) {
        av_bsf_free(&bsf_ctx);
        return nullptr;
    }
    return bsf_ctx;
}


int applyBitstream(AVBSFContext *bsf, AVPacket *packet) {
    if (bsf == nullptr)
        return -1;
    int ret = 0;
    ret = av_bsf_send_packet(bsf, packet);
    if (ret < 0) {
        LOGE("av_bsf_send_packet error %d", ret);
        return ret;
    }
    av_packet_unref(packet);
    while (!ret)
        ret = av_bsf_receive_packet(bsf, packet);
    if (ret < 0 && (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)) {
        LOGE("h264_mp4toannexb filter  failed to receive output packet\n");
        return ret;
    }
    ret = 0;
    return ret;
}