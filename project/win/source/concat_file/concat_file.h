#pragma once

#include "core/media_common/media_struct.h"
#include "core/media_common/bsf_filter.h"
#include "core/media_common/raw_stream_parser.h"

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
        SMALL,
        LARGE
    };

    typedef struct VideoPadding {
        VStreamType streamType;
        int64_t start_pts = AV_NOPTS_VALUE;
        int64_t end_pts = AV_NOPTS_VALUE;
        VideoPadding(VStreamType type, int64_t startpts, int64_t endpts) :
            streamType(type), start_pts(startpts), end_pts(endpts)
        {
        }
    }VideoPadding;


    typedef struct InputFile {
        std::map<int64_t, VideoPadding> paddingInfo;
        std::unique_ptr<AVFormatContext, AVFormatContextInputDeleter> inputFormat_ctx = nullptr;
        AVStream *v_stream = nullptr;
        AVStream *a_stream = nullptr;
        AVFrame *frame = av_frame_alloc();
        std::deque<std::unique_ptr<AVPacket, AVPacketDeleter>> packet_queue;
        bool eof = false;
        InputFile() {

        }

    }InputFile;

    typedef struct OutputFile {
        std::unique_ptr<AVFormatContext, AVFormatContextInputDeleter> outputFormat_ctx = nullptr;
        AVStream *v_stream = nullptr;
        AVStream *a_stream = nullptr;
        OutputFile() {
        }

    }OutputFile;

    typedef struct Context {
        InputFile *first_input_ctx;
        InputFile *second_input_ctx;
        OutputFile *output_ctx;
        Context() {
            first_input_ctx = new InputFile();
            second_input_ctx = new InputFile();
            output_ctx = new OutputFile();
        }


    }Context;


    static int open_input_file(const char *filename, InputFile *input_ctx)
    {
        int ret;
        unsigned int i;

        AVFormatContext *ifmt_ctx = NULL;
        if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
            return ret;
        }

        if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
            return ret;
        }

        for (i = 0; i < ifmt_ctx->nb_streams; i++) {
            AVStream *stream = ifmt_ctx->streams[i];
            AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
            AVCodecContext *codec_ctx;
            if (!dec) {
                av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
                //return AVERROR_DECODER_NOT_FOUND;
                continue;
            }
            codec_ctx = stream->codec;
            if (!codec_ctx) {
                av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
                return AVERROR(ENOMEM);
            }
            ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                    "for stream #%u\n", i);
                return ret;
            }
            /* Reencode video & audio and remux subtitles etc. */
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                    codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
                /* Open decoder */
                ret = avcodec_open2(codec_ctx, dec, NULL);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                    return ret;
                }
            }

            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                input_ctx->v_stream = stream;
            }
            else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                input_ctx->a_stream = stream;
            }

        }

        input_ctx->inputFormat_ctx.reset(ifmt_ctx);
        av_dump_format(ifmt_ctx, 0, filename, 0);
        return 0;
    }

    static int open_output_file(AVFormatContext *ifmt_ctx, const char *filename, OutputFile *output_ctx)
    {
        AVStream *out_stream;
        AVStream *in_stream;
        AVCodecContext *dec_ctx, *enc_ctx;
        AVCodec *encoder;
        int ret;
        unsigned int i;

        AVFormatContext *ofmt_ctx = NULL;
        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
        if (!ofmt_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
            return AVERROR_UNKNOWN;
        }


        for (i = 0; i < ifmt_ctx->nb_streams; i++) {


            in_stream = ifmt_ctx->streams[i];
            dec_ctx = in_stream->codec;


            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                /* in this example, we choose transcoding to same codec */
                encoder = avcodec_find_encoder(dec_ctx->codec_id);
                if (!encoder) {
                    av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
                    return AVERROR_INVALIDDATA;
                }
                out_stream = avformat_new_stream(ofmt_ctx, encoder);
                if (!out_stream) {
                    av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
                    return AVERROR_UNKNOWN;
                }
                enc_ctx = out_stream->codec; //avcodec_alloc_context3(encoder);

                if (!enc_ctx) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
                    return AVERROR(ENOMEM);
                }

                /* In this example, we transcode to same properties (picture size,
                 * sample rate etc.). These properties can be changed for output
                 * streams easily using filters */
                if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                    enc_ctx->height = dec_ctx->height;
                    enc_ctx->width = dec_ctx->width;
                    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                    /* take first format from list of supported formats */
                    if (encoder->pix_fmts)
                        enc_ctx->pix_fmt = encoder->pix_fmts[0];
                    else
                        enc_ctx->pix_fmt = dec_ctx->pix_fmt;
                    /* video time_base can be set to whatever is handy and supported by encoder */
                    enc_ctx->time_base = av_inv_q(dec_ctx->framerate);

                    output_ctx->v_stream = out_stream;
                }
                else {
                    enc_ctx->sample_rate = dec_ctx->sample_rate;
                    enc_ctx->channel_layout = dec_ctx->channel_layout;
                    enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                    /* take first format from list of supported formats */
                    enc_ctx->sample_fmt = encoder->sample_fmts[0];
                    enc_ctx->time_base = { 1, enc_ctx->sample_rate };

                    output_ctx->a_stream = out_stream;
                }

                /* Third parameter can be used to pass settings to encoder */
                ret = avcodec_open2(enc_ctx, encoder, NULL);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
                    return ret;
                }
                ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
                    return ret;
                }
                if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                out_stream->time_base = enc_ctx->time_base;

            }
            else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
                av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
                return AVERROR_INVALIDDATA;
            }
            else {
                /* if this stream must be remuxed */
                ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
                    return ret;
                }
                out_stream->time_base = in_stream->time_base;
            }

        }
        av_dump_format(ofmt_ctx, 0, filename, 1);

        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
                return ret;
            }
        }

        /* init muxer, write output file header */
        ret = avformat_write_header(ofmt_ctx, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
            return ret;
        }
        output_ctx->outputFormat_ctx.reset(ofmt_ctx);
        return 0;
    }

}