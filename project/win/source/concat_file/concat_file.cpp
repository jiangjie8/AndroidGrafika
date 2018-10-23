#include "concat_file.h"

namespace av {
std::map<int64_t, VideoPadding> probe_sei_info(const char *input) {

    AVDemuxer demuxer;
    std::unique_ptr<AVBSFContext, AVBSFContextDeleter> mp4H264_bsf = nullptr;
        
    if (demuxer.openInputFormat(input)  < 0)
        return std::map<int64_t, VideoPadding>();
        
    std::string  extensions = demuxer.getInputFormat()->extensions;
    if (extensions.find("mp4") || extensions.find("mov") || extensions.find("m4a")) {
        if (demuxer.getStream(AVMEDIA_TYPE_VIDEO) != nullptr) {
            mp4H264_bsf.reset(initBsfFilter(demuxer.getStream(AVMEDIA_TYPE_VIDEO)->codecpar, "h264_mp4toannexb"));
        }
    }
    std::map<int64_t, VideoPadding>paddingInfo;
    VStreamType stream_type = VStreamType::SMALL;
    int64_t previous_pts = AV_NOPTS_VALUE;

    while (true)
    {
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        if (demuxer.readPacket(packet.get()) < 0)
            break;
        int stream_index = packet->stream_index;
        enum AVMediaType media_type = demuxer.getStream(stream_index)->codecpar->codec_type;
        if (media_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        else if (!(packet->flags & AV_PKT_FLAG_KEY)) {
            previous_pts = packet->pts + packet->duration;
            if (paddingInfo.size() > 0) {
                auto iterator = --paddingInfo.end();
                iterator->second.end_pts = previous_pts > iterator->second.end_pts ? previous_pts :iterator->second.end_pts;
            }
            continue;
        }
        //previous_pts = packet->pts + packet->duration;
            
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
        if (strncmp((const char*)vp_sei, VP_SMALL, sizeof(VP_SMALL)) == 0) {
            stream_type = VStreamType::SMALL;
        }
        else if (strncmp((const char*)vp_sei, VP_LARGER, sizeof(VP_LARGER)) == 0) {
            stream_type = VStreamType::LARGE;
        }
        if (paddingInfo.size() == 0) {
            paddingInfo.insert(std::make_pair(packet->pts, VideoPadding(stream_type, packet->pts, 0)));
        }
        else {
            auto iterator = --paddingInfo.end();
            if (iterator->second.streamType != stream_type || iterator->second.end_pts < packet->pts) {
                paddingInfo.insert(std::make_pair(packet->pts, VideoPadding(stream_type, packet->pts, 0)));
            }
        }
    }
    LOGW("===== input  %s =====\n", input);
    for (auto &info : paddingInfo) {
        LOGW("%s   start %8lld ; end %8lld\n", info.second.streamType == VStreamType::SMALL ? "small" : "large",
            info.second.start_pts, info.second.end_pts);
    }
    return paddingInfo;
}


int MergerCtx::openInputStreams(const char *file1, const char *file2) 
{
    int ret = 0;

    m_sei_info1 = probe_sei_info(file1);
    m_sei_info2 = probe_sei_info(file2);

    m_aStream1.reset(new AVDemuxer());
    if (m_aStream1->openInputFormat(file1) < 0) {
        m_aStream1.reset();
        return -1;
    }
    if (m_aStream1->getStream(AVMEDIA_TYPE_AUDIO) == nullptr) {
        m_aStream1.reset();
    }

    m_vStream1.reset(new AVDemuxer());
    if (m_vStream1->openInputFormat(file1) < 0) {
        m_vStream1.reset();
        return -1;
    }

    m_vStream2.reset(new AVDemuxer());
    if (m_vStream2->openInputFormat(file2) < 0) {
        m_vStream2.reset();
        return -1;
    }
    return ret;
}

int MergerCtx::openOutputFile(const char *file) 
{
    int ret = 0;
    m_output.reset(new AVMuxer());
    if (( ret = m_output->openOutputFormat(file)) < 0)
        m_output.reset();
    return ret;
}

int MergerCtx::cfgCodec()
{
    int ret = -1;
    auto param = m_vStream1->getCodecParameters(AVMEDIA_TYPE_VIDEO);
    if (param) {
        m_frame_rate = { m_vStream1->video_codecParam.frame_rate_num , m_vStream1->video_codecParam.frame_rate_den };
        m_decodeV1.reset(new AVDecoder());
        m_decodeV1->cfgCodec(param);
        auto decode_src = m_vStream1->getStream(AVMEDIA_TYPE_VIDEO)->codec;
        auto decode_dst = m_decodeV1->getCodecContext();
        decode_dst->time_base = decode_src->time_base;
        decode_dst->ticks_per_frame = decode_src->ticks_per_frame;
        if ((ret = m_decodeV1->openCodec()) < 0) {
            m_decodeV1.reset();
            return ret;
        }
            
    }

    param = m_vStream2->getCodecParameters(AVMEDIA_TYPE_VIDEO);
    if (param) {
        m_decodeV2.reset(new AVDecoder());
        m_decodeV2->cfgCodec(param);
        auto decode_src = m_vStream2->getStream(AVMEDIA_TYPE_VIDEO)->codec;
        auto decode_dst = m_decodeV2->getCodecContext();
        decode_dst->time_base = decode_src->time_base;
        decode_dst->ticks_per_frame = decode_src->ticks_per_frame;
        if ((ret = m_decodeV2->openCodec()) < 0) {
            m_decodeV2.reset();
            return ret;
        }
    }

    if (m_decodeV1 && m_decodeV2) {
        m_encodeV.reset(new AVEncoder());
        std::unique_ptr<AVCodecParameters, AVCodecParametersDeleter>
            encode_param(avcodec_parameters_alloc());
        avcodec_parameters_copy(encode_param.get(), param);
        m_encodeV->cfgCodec(encode_param.get(), "libx264");
        if (m_output->getOutputFormat()->flags & AVFMT_GLOBALHEADER)
            m_encodeV->getCodecContext()->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        m_encodeV->getCodecContext()->time_base = m_decodeV1->getCodecContext()->time_base;
        m_encodeV->getCodecContext()->ticks_per_frame = m_decodeV1->getCodecContext()->ticks_per_frame;

        AVDictionary *options = nullptr;
        av_dict_set(&options, "tune", "film", 0);
        av_dict_set(&options, "preset", "veryfast", 0);
        av_dict_set(&options, "x264-params", "crf=22:aq-mode=1", 0);
        if ((ret = m_encodeV->openCodec(&options)) < 0)
            m_encodeV.reset();
        av_dict_free(&options);
        if (ret < 0)
            return ret;

        m_filterGraph.reset(new FilterGraph());
        char filter_spec[512] = { 0 };
        snprintf(filter_spec, sizeof(filter_spec) - 1,
            "[in]drawtext=text='%s':x=w/2-100:y=0:fontcolor=red:fontsize=25,scale=w=%d:h=%d:flags=%d[out]",
            "Small Video",
            encode_param->width, encode_param->height,
            SWS_FAST_BILINEAR);
        ret = m_filterGraph->initFilter(m_decodeV1->getCodecContext(), m_encodeV->getCodecContext(), filter_spec);
        if (ret < 0) {
            m_filterGraph = nullptr;
        }
    }

    return ret;
}


int MergerCtx::getVideoFrame(AVDemuxer *demuxer, AVDecoder *decoder, AVFrame *frame)
{
    static int index = 0;
    int ret = 0;
    while (true)
    {
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        while (true) {
            ret = demuxer->readPacket(packet.get());
            if (ret < 0)
                break;
            if (demuxer->getStream(packet->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                break;
        }
            
        if (ret == AVERROR_EOF) {
            ret = decoder->flushCodec(frame);
            frame->pts = av_rescale_q(frame->pts, decoder->getCodecContext()->time_base, { 1, AV_TIME_BASE });
                     
            return ret;
        }
        else if (ret < 0) {
            break;
        }
 
        av_packet_rescale_ts(packet.get(), { 1, AV_TIME_BASE }, decoder->getCodecContext()->time_base);
        ret = decoder->sendPacket(packet.get());
        if (ret < 0)
            break;
        ret = decoder->receiveFrame(frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        }
        else if (ret < 0) {
            break;
        }
        vp_frame_rescale_ts(frame, decoder->getCodecContext()->time_base, { 1, AV_TIME_BASE });
        break;
    }
    return ret;
}

int MergerCtx::writeAudioPacket(AVDemuxer *input, AVMuxer  *output, int64_t pts_flag, int index) {
    while (true)
    {
        int ret = 0;
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        while (true) {
            ret = input->readPacket(packet.get());
            if (ret < 0)
                return ret;
            if (input->getStream(packet->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                break;
        }
        if (ret == 0) {
            int64_t pts = packet->pts;
            int64_t duration = packet->duration;
            packet->stream_index = index;
            output->writePacket(packet.get());
            if (pts + duration >= pts_flag && pts_flag != AV_NOPTS_VALUE) {
                break;
            }
        }
    }

    return 0;
}
int MergerCtx::frameScale(AVFrame *frame) {
    if (m_filterGraph == nullptr)
        return -1;
    int ret = 0;
    if (frame->width != m_encodeV->getCodecContext()->width ||
        frame->height != m_encodeV->getCodecContext()->height ||
        frame->format != m_encodeV->getCodecContext()->pix_fmt) {
        m_filterGraph->addFrame(frame);
        av_frame_unref(frame);
        ret = m_filterGraph->getFrame(frame);
        if (ret < 0) {
            LOGE("filter get frame error");
        }
    }
    return ret;
}


int MergerCtx::mergerLoop() {
    if (m_vStream1 == nullptr ||
        m_vStream2 == nullptr ||
        m_output == nullptr ||
        m_decodeV1 == nullptr ||
        m_decodeV2 == nullptr ||
        m_encodeV == nullptr) {
        return -1;
    }

    int ret = 0;
    int64_t last_one_pts = AV_NOPTS_VALUE;

    AVCodecParameters *codecpar = avcodec_parameters_alloc();
    avcodec_parameters_from_context(codecpar, m_encodeV->getCodecContext());
    m_videIndex = m_output->addStream(codecpar);
    avcodec_parameters_free(&codecpar);

    if(m_aStream1)
        m_audioIndex = m_output->addStream(m_aStream1->getCodecParameters(AVMEDIA_TYPE_AUDIO));
    ret = m_output->writeHeader();

    while (ret == 0)
    {
        bool use2 = false;
        std::unique_ptr<AVFrame, AVFrameDeleter> vframe1(av_frame_alloc());
        ret = getVideoFrame(m_vStream1.get(), m_decodeV1.get(), vframe1.get());
        if (ret < 0)
            break;

        for (auto bitrateInfo : m_sei_info2) {
            VStreamType type = bitrateInfo.second.streamType;
            int64_t pts_start = bitrateInfo.second.start_pts;
            int64_t pts_end = bitrateInfo.second.end_pts;
            if (vframe1->pts >= pts_start && vframe1->pts < pts_end) {
                use2 = type == VStreamType::LARGE ? true : false;
                break;
            }
        }

        std::unique_ptr<AVFrame, AVFrameDeleter> frame;
        if (use2) {
            std::unique_ptr<AVFrame, AVFrameDeleter> vframe2(av_frame_alloc());
            while (true) {
                ret = getVideoFrame(m_vStream2.get(), m_decodeV2.get(), vframe2.get());
                if (ret == AVERROR_EOF) {
                    ret = 0;
                    use2 = false;
                    break;
                }
                else if (ret < 0 || vframe2->pts >= vframe1->pts) {
                    break;
                } 
            }
            if (vframe2->pict_type != AV_PICTURE_TYPE_I) {
                vframe2->pict_type = AV_PICTURE_TYPE_NONE;
            }
            frame.reset(vframe2.release());
        }
        if(!use2){
            vframe1->pict_type = AV_PICTURE_TYPE_NONE;
            frame.reset(vframe1.release());
        }


        if (ret < 0)
            break;
        if (frame->pts <= last_one_pts) {
            LOGE("drop  %lld  <= %lld \n", frame->pts, last_one_pts);
            continue;
        }

        ret = frameScale(frame.get());
        if (ret < 0)
            break;
            
        last_one_pts = frame->pts;
        ret = encodeWriteFrame(m_encodeV.get(), frame.get());
        if (ret == AVERROR(EAGAIN)) {
            ret = 0;
            continue;
        }
        else if (ret < 0) {
            break;
        }


    }

    //flush encode
    while (true)
    {
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        ret = m_encodeV->flushCodec(packet.get());
        if (ret < 0)
            break;
        av_packet_rescale_ts(packet.get(), m_encodeV->getCodecContext()->time_base, { 1, AV_TIME_BASE });
        int64_t vpts = packet->pts;
        packet->duration = av_rescale_q(1, av_inv_q(m_frame_rate), { 1, AV_TIME_BASE });
        packet->stream_index = m_videIndex;
        m_output->writePacket(packet.get());

        if (m_aStream1)
            writeAudioPacket(m_aStream1.get(), m_output.get(), vpts, m_audioIndex);
    }

    if (m_aStream1)
        writeAudioPacket(m_aStream1.get(), m_output.get(), AV_NOPTS_VALUE, m_audioIndex);
    ret = m_output->closeOutputForamt();
    return ret;
}

int MergerCtx::encodeWriteFrame(AVEncoder *encoder, AVFrame *frame) {
    int ret = 0;
    std::unique_ptr<AVPacket, AVPacketDeleter> vpacket(av_packet_alloc());
    //frame->pict_type = AV_PICTURE_TYPE_NONE;
    vp_frame_rescale_ts(frame, { 1, AV_TIME_BASE }, encoder->getCodecContext()->time_base);
    ret = encoder->sendFrame(frame);
    ret = encoder->receivPacket(vpacket.get());
    if (ret < 0)
        return ret;
        
    av_packet_rescale_ts(vpacket.get(), m_encodeV->getCodecContext()->time_base, { 1, AV_TIME_BASE });
    int64_t vpts = vpacket->pts;
    vpacket->duration = av_rescale_q(1, av_inv_q(m_frame_rate), { 1, AV_TIME_BASE });
    vpacket->stream_index = m_videIndex;
    m_output->writePacket(vpacket.get());
    if (m_aStream1)
        writeAudioPacket(m_aStream1.get(), m_output.get(), vpts, m_audioIndex);
    return 0;
}
}




using namespace av;
int main(int argc, char *argv[]) {
    int ret = 0;
    if (parser_option(argc, argv) < 0) {
        LOGE("command : ");
        for (int i = 0; i < argc; ++i) {
            LOGE("%s ", argv[i]);
        }
        LOGE("\n");
        return -1;
    }
        
    const char *in1 = command_t.input1.c_str();
    const char *in2 = command_t.input2.c_str();
    MergerCtx ctx;
    if ((ret = ctx.openInputStreams(in1, in2)) < 0)
        return ret;
    if ((ret = ctx.openOutputFile(command_t.output.c_str())) < 0)
        return ret;
    if ((ret = ctx.cfgCodec()) < 0)
        return ret;
    return ctx.mergerLoop();
}
