#include "concat_file.h"

using namespace std;



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
                previous_pts = packet->pts;
                continue;
            }
            previous_pts = packet->pts;
            
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
            LOGW("find:   %s  %lld\n", vp_sei, packet->pts);
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
                iterator->second.end_pts = packet->pts;
                if (iterator->second.streamType != stream_type) {
                    paddingInfo.insert(std::make_pair(packet->pts, VideoPadding(stream_type, packet->pts, 0)));
                }
            }
        }
        auto iterator = --paddingInfo.end();
        iterator->second.end_pts = previous_pts;
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
        return 0;
    }

    int MergerCtx::openOutputFile(const char *file) 
    {
        m_output.reset(new AVMuxer());
        if (m_output->openOutputFormat(file) < 0)
            m_output.reset();
        return 0;
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
            if ((ret = m_decodeV1->openCodec()) < 0)
                m_decodeV1.reset();
        }

        param = m_vStream2->getCodecParameters(AVMEDIA_TYPE_VIDEO);
        if (param) {
            m_decodeV2.reset(new AVDecoder());
            m_decodeV2->cfgCodec(param);
            auto decode_src = m_vStream2->getStream(AVMEDIA_TYPE_VIDEO)->codec;
            auto decode_dst = m_decodeV2->getCodecContext();
            decode_dst->time_base = decode_src->time_base;
            decode_dst->ticks_per_frame = decode_src->ticks_per_frame;
            if ((ret = m_decodeV2->openCodec()) < 0)
                m_decodeV2.reset();
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
            if ((ret = m_encodeV->openCodec()) < 0)
                m_encodeV.reset();
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
                int64_t pts_start = bitrateInfo.first;
                VStreamType type = bitrateInfo.second.streamType;
                if (vframe1->pts >= pts_start && type == VStreamType::LARGE) {
                    use2 = true;
                }
                if (vframe1->pts >= pts_start && type == VStreamType::SMALL) {
                    use2 = false;
                }
            }

            std::unique_ptr<AVFrame, AVFrameDeleter> frame;
            if (use2) {
                std::unique_ptr<AVFrame, AVFrameDeleter> vframe2(av_frame_alloc());
                while (true) {
                    ret = getVideoFrame(m_vStream2.get(), m_decodeV2.get(), vframe2.get());
                    if (ret < 0 || vframe2->pts >= vframe1->pts)
                        break;
                }
                frame.reset(vframe2.release());
            }
            else {
                frame.reset(vframe1.release());
            }
            if (ret < 0)
                break;
            if (frame->pts <= last_one_pts) {
                LOGE("drop  %lld  <= %lld \n", frame->pts, last_one_pts);
                continue;
            }
            
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
        m_output->closeOutputForamt();
        return 0;
    }

    int MergerCtx::encodeWriteFrame(AVEncoder *encoder, AVFrame *frame) {
        int ret = 0;
        std::unique_ptr<AVPacket, AVPacketDeleter> vpacket(av_packet_alloc());
        frame->pict_type = AV_PICTURE_TYPE_NONE;
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
int main() {
    const char *in1 = R"(./output.mp4)";
    const char *in2 = R"(./output1.mp4)";
    MergerCtx ctx;
    ctx.openInputStreams(in1, in2);
    ctx.openOutputFile(R"(./test.mp4)");
    ctx.cfgCodec();
    ctx.mergerLoop();
    return 0;
   

}
