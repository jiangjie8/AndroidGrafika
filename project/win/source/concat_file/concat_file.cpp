#include "concat_file.h"

using namespace std;


namespace av {
    std::map<int64_t, VideoPadding> probe_sei_info(const char *input) {
        std::unique_ptr<AVFormatContext, AVFormatContextInputDeleter> inputFormat_ctx = nullptr;
        std::unique_ptr<AVBSFContext, AVBSFContextDeleter> mp4H264_bsf = nullptr;
        AVFormatContext *temp_format = nullptr;
        avformat_open_input(&temp_format, input, nullptr, nullptr);
        if (temp_format == nullptr)
            return std::map<int64_t, VideoPadding>();
        inputFormat_ctx.reset(temp_format);
        temp_format = nullptr;
        av_find_best_stream(inputFormat_ctx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        std::string  extensions = inputFormat_ctx->iformat->extensions;
        if (extensions.find("mp4")
            || extensions.find("mov")
            || extensions.find("m4a")) {
            for (int i = 0; i < inputFormat_ctx->nb_streams; i++) {
                AVStream *stream = inputFormat_ctx->streams[i];
                if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    mp4H264_bsf.reset(initBsfFilter(stream->codecpar, "h264_mp4toannexb"));
                    break;
                }
            }
        }

        std::map<int64_t, VideoPadding>paddingInfo;
        VStreamType stream_type = VStreamType::SMALL;
        int64_t previous_pts = AV_NOPTS_VALUE;

        while (true)
        {
            std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
            if (av_read_frame(inputFormat_ctx.get(), packet.get()) < 0)
                break;
            int stream_index = packet->stream_index;
            enum AVMediaType media_type = inputFormat_ctx->streams[stream_index]->codecpar->codec_type;
            if (media_type != AVMEDIA_TYPE_VIDEO) {
                continue;
            }
            else if (!(packet->flags & AV_PKT_FLAG_KEY)) {
                previous_pts = av_rescale_q(packet->pts, inputFormat_ctx->streams[stream_index]->time_base, { 1, AV_TIME_BASE });
                continue;
            }
            AVRational stream_timebase = inputFormat_ctx->streams[stream_index]->time_base;
            applyBitstream(mp4H264_bsf.get(), packet.get());
            std::vector<std::string> vp_sei_queue = vp_sei_user_data(packet->data, packet->data + packet->size);
            const char *vp_sei = nullptr;
            for (auto &sei : vp_sei_queue) {
                if (!sei.find(VP_SMALL) || !sei.find(VP_LARGER)) {
                    vp_sei = sei.c_str();
                }
            }
            
            if(vp_sei == nullptr)
                continue;
            av_packet_rescale_ts(packet.get(), stream_timebase, { 1, AV_TIME_BASE });
            std::cout << "find:  " << vp_sei << "  " << packet->pts << std::endl;

            if (strncmp((const char*)vp_sei, VP_SMALL, sizeof(VP_SMALL)) == 0) {
                stream_type = VStreamType::SMALL;
            }
            else if (strncmp((const char*)vp_sei, VP_LARGER, sizeof(VP_LARGER)) == 0) {
                stream_type = VStreamType::LARGE;
            }
            if (paddingInfo.size() == 0) {

            }
            else {
                auto iterator = --paddingInfo.end();
                iterator->second.end_pts = packet->pts;
            }
            paddingInfo.insert(std::make_pair(packet->pts, VideoPadding(stream_type, packet->pts, 0)));

        }
        auto iterator = --paddingInfo.end();
        iterator->second.end_pts = previous_pts;
        return paddingInfo;
    }



    void initContext(Context *contex) {
        InputFile *first_input_ctx = contex->first_input_ctx;
        InputFile *second_input_ctx = contex->second_input_ctx;

    }

    int getVideoFrame(InputFile *input_ctx)
    {
        int ret = 0;
        if (input_ctx->frame->data[0] == nullptr) {
            while (true)
            {
                std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
                if (!input_ctx->eof) {
                    ret = av_read_frame(input_ctx->inputFormat_ctx.get(), packet.get());
                    if (ret < 0 && ret != AVERROR_EOF) {
                        break;
                    }
                    else if (ret == AVERROR_EOF) {
                        input_ctx->eof = true;
                        ret = avcodec_send_packet(input_ctx->v_stream->codec, nullptr);
                    }
                }

                if (!input_ctx->eof) {
                    int stream_index = packet->stream_index;
                    enum AVMediaType media_type = input_ctx->inputFormat_ctx->streams[stream_index]->codecpar->codec_type;
                    if (media_type == AVMEDIA_TYPE_AUDIO) {
                        while (input_ctx->packet_queue.size() > 10) {
                            input_ctx->packet_queue.pop_front();
                        }
                        av_packet_rescale_ts(packet.get(), input_ctx->inputFormat_ctx->streams[stream_index]->time_base,
                            { 1, AV_TIME_BASE });
                        input_ctx->packet_queue.push_back(std::move(packet));
                    }
                    if (media_type != AVMEDIA_TYPE_VIDEO)
                        continue;
                    av_packet_rescale_ts(packet.get(), input_ctx->inputFormat_ctx->streams[stream_index]->time_base,
                        { 1, AV_TIME_BASE });
                    ret = avcodec_send_packet(input_ctx->v_stream->codec, packet.get());
                    if (ret < 0 && ret != AVERROR_EOF)
                        break;
                }

                ret = avcodec_receive_frame(input_ctx->v_stream->codec, input_ctx->frame);
                if (ret < 0 && ret != AVERROR(EAGAIN))
                    break;
                else if (ret == AVERROR(EAGAIN))
                    continue;
                //printf(" %p %lld\n", input_ctx, input_ctx->frame->pts);
                //av_frame_move_ref(frame, input_ctx->frame);
                //av_frame_unref(input_ctx->frame);
                break;
            }

        }
        else {
            //av_frame_move_ref(frame, input_ctx->frame);
        }

        return ret;
    }

    void releaseFrame(InputFile *input_ctx, AVFrame *frame) {
        if (frame != nullptr) {
            av_frame_unref(frame);
            av_frame_move_ref(frame, input_ctx->frame);
        }
        av_frame_unref(input_ctx->frame);
    }

    int encodeFrame(OutputFile *output_ctx, AVFrame *frame) {
        int ret = 0;
        frame->pts = av_rescale_q(frame->pts, { 1, AV_TIME_BASE }, output_ctx->v_stream->codec->time_base);
        ret = avcodec_send_frame(output_ctx->v_stream->codec, frame);
        if (ret < 0)
            return ret;
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        ret = avcodec_receive_packet(output_ctx->v_stream->codec, packet.get());
        if (ret == 0) {
            av_packet_rescale_ts(packet.get(), output_ctx->v_stream->codec->time_base,
                output_ctx->v_stream->time_base);
            ret = av_interleaved_write_frame(output_ctx->outputFormat_ctx.get(), packet.get());
        }
        else if (ret == AVERROR(EAGAIN)) {
            ret = 0;
        }
        return ret;
    }

    int copyAudioStream(OutputFile *output_ctx, AVPacket *packet) {

        av_packet_rescale_ts(packet, { 1, AV_TIME_BASE }, output_ctx->a_stream->time_base);
        return av_interleaved_write_frame(output_ctx->outputFormat_ctx.get(), packet);
    }


    void flushEncode(OutputFile *output_ctx) {
        int ret = 0;
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        while (true) {

            while ((ret = avcodec_receive_packet(output_ctx->v_stream->codec, packet.get())) == AVERROR(EAGAIN)) {
                ret = avcodec_send_frame(output_ctx->v_stream->codec, nullptr);
                if (ret < 0) {
                    printf("error\n");
                }
            }
            if (ret == AVERROR_EOF)
                return;
            if (ret == 0) {
                LOGW("flush %lld\n", av_rescale_q(packet->pts, output_ctx->v_stream->codec->time_base, { 1, AV_TIME_BASE }));
                av_packet_rescale_ts(packet.get(), output_ctx->v_stream->codec->time_base,
                    output_ctx->v_stream->time_base);
                ret = av_interleaved_write_frame(output_ctx->outputFormat_ctx.get(), packet.get());

            }
        }

    }



}

using namespace av;
int main() {

    Context *ctx = new Context();
    ctx->first_input_ctx->paddingInfo = probe_sei_info(R"(D:\videoFile\test\output.mp4)");
    ctx->second_input_ctx->paddingInfo = probe_sei_info(R"(D:\videoFile\test\output1.mp4)");

    open_input_file(R"(D:\videoFile\test\output.mp4)", ctx->first_input_ctx);
    open_input_file(R"(D:\videoFile\test\output1.mp4)", ctx->second_input_ctx);

    open_output_file(ctx->first_input_ctx->inputFormat_ctx.get(), R"(D:\videoFile\test\test.mp4)",
        ctx->output_ctx);

    int64_t last_pts = 0;
    while (true) {

        std::unique_ptr<AVFrame, AVFrameDeleter> frame1(av_frame_alloc());
        std::unique_ptr<AVFrame, AVFrameDeleter> frame2(av_frame_alloc());

        std::unique_ptr<AVFrame, AVFrameDeleter> frame(av_frame_alloc());

        if (getVideoFrame(ctx->first_input_ctx) < 0)
            break;
        getVideoFrame(ctx->second_input_ctx);
        //         releaseFrame(ctx->first_input_ctx, frame1.get());
        //         releaseFrame(ctx->second_input_ctx, frame2.get());

        bool use2 = false;
        for (auto bitrateInfo : ctx->second_input_ctx->paddingInfo) {
            int64_t pts_start = bitrateInfo.first;
            VStreamType type = bitrateInfo.second.streamType;
            if ((ctx->first_input_ctx->frame->pts >= pts_start || ctx->second_input_ctx->frame->pts >= pts_start)
                && type == VStreamType::LARGE) {
                use2 = true;
            }

            if ((ctx->first_input_ctx->frame->pts >= pts_start || ctx->second_input_ctx->frame->pts >= pts_start)
                && type == VStreamType::SMALL) {
                use2 = false;
                last_pts = pts_start;
            }
        }

        if (!use2) {
            while (true) {
                getVideoFrame(ctx->first_input_ctx);
                if (ctx->first_input_ctx->frame->pts >= last_pts)
                    break;
                releaseFrame(ctx->first_input_ctx, nullptr);
            }
            releaseFrame(ctx->first_input_ctx, frame.get());
            if (ctx->first_input_ctx->packet_queue.size() > 0) {
                std::unique_ptr<AVPacket, AVPacketDeleter> packet(ctx->first_input_ctx->packet_queue.front().release());
                ctx->first_input_ctx->packet_queue.pop_front();
                copyAudioStream(ctx->output_ctx, packet.get());
            }
            LOGW("first %lld\n", frame->pts);
        }
        else {
            while (true) {
                getVideoFrame(ctx->second_input_ctx);
                if (ctx->second_input_ctx->frame->pts >= ctx->first_input_ctx->frame->pts)
                    break;
                releaseFrame(ctx->second_input_ctx, nullptr);
            }

            releaseFrame(ctx->second_input_ctx, frame.get());
            if (ctx->second_input_ctx->packet_queue.size() > 0) {
                std::unique_ptr<AVPacket, AVPacketDeleter> packet(ctx->second_input_ctx->packet_queue.front().release());
                ctx->second_input_ctx->packet_queue.pop_front();
                copyAudioStream(ctx->output_ctx, packet.get());
            }

            LOGW("second %lld\n", frame->pts);
            //releaseFrame(ctx->first_input_ctx, nullptr);
        }
        encodeFrame(ctx->output_ctx, frame.get());
        int a = 10;

    }


    flushEncode(ctx->output_ctx);

    av_write_trailer(ctx->output_ctx->outputFormat_ctx.get());
    return 0;
}