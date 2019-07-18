#include "core/media_common/raw_filter.h"

namespace av {
int FilterGraph::initFilter(const AVCodecContext *dec_ctx, const AVCodecContext *enc_ctx, const char *filter_spec) {
    char args[512] = {0};
    int ret = 0;
    const AVFilter *buffersrc = nullptr;
    const AVFilter *buffersink = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    m_graph_filter.reset(nullptr);
    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        buffersrc = avfilter_get_by_name("buffer");
        buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            LOGE("filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            dec_ctx->time_base.num, dec_ctx->time_base.den,
            dec_ctx->sample_aspect_ratio.num,
            dec_ctx->sample_aspect_ratio.den);

        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
            args, nullptr, filter_graph);
        if (ret < 0) {
            LOGE("Cannot create buffer source\n");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
            nullptr, nullptr, filter_graph);
        if (ret < 0) {
            LOGE("Cannot create buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
            (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
            AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output pixel format\n");
            goto end;
        }
    }
    else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink) {
            LOGE("filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
            av_get_sample_fmt_name(dec_ctx->sample_fmt),
            dec_ctx->channel_layout);
        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
            args, nullptr, filter_graph);
        if (ret < 0) {
            LOGE("Cannot create audio buffer source\n");
            goto end;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
            nullptr, nullptr, filter_graph);
        if (ret < 0) {
            LOGE("Cannot create audio buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
            (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
            AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output sample format\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
            (uint8_t*)&enc_ctx->channel_layout,
            sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output channel layout\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
            (uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
            AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            LOGE("Cannot set output sample rate\n");
            goto end;
        }
    }
    else {
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
        &inputs, &outputs, nullptr)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0)
        goto end;

    av_buffersink_set_frame_size(buffersink_ctx, enc_ctx->frame_size);
    /* Fill FilteringContext */
    m_buffersrc_ctx = buffersrc_ctx;
    m_buffersink_ctx = buffersink_ctx;
    m_graph_filter.reset(filter_graph);
    filter_graph = nullptr;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&filter_graph);
    return ret;
}



}