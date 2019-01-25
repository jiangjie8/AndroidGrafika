#include "concat_file.h"

constexpr char *Small_LOGO = "/opt/vcloud/smallFlag.png";
constexpr float Flexible = 0.3;
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
    int64_t last_pts = AV_NOPTS_VALUE;
    int64_t duration = av_rescale_q(1, AVRational{ demuxer.video_codecParam.frame_rate_den, demuxer.video_codecParam.frame_rate_num }, { 1, AV_TIME_BASE });

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
            //last_pts = packet->pts + packet->duration;
            last_pts = packet->pts + duration;
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
    if (paddingInfo.size() > 0) {
        auto iterator = --paddingInfo.end();
        iterator->second.end_pts = last_pts > iterator->second.end_pts ? last_pts : iterator->second.end_pts;
    }
    //LOGW("===== input  %s =====\n", input);
    //for (auto &info : paddingInfo) {
    //    LOGW("%s   start %8lld ; end %8lld\n", info.second.streamType == VStreamType::SMALL ? "small" : "large",
    //        info.second.start_pts, info.second.end_pts);
    //}
    return paddingInfo;
}

std::vector<std::string> split(const char *s, char delimiter)
{
    std::string str(s);
    std::vector<std::string> tokens;
    std::string token;
    auto p0 = 0;
    auto p1 = str.find(delimiter);
    while(p1 != std::string::npos)
    {
        token = str.substr(p0, p1 - p0);
        tokens.push_back(token);
        p0 = p1 + 1;
        p1 = str.find(delimiter, p0);
        if (p1 == std::string::npos) {
            token = str.substr(p0);
            tokens.push_back(token);
        }
    }

    if (p0 == 0) {
        tokens.push_back(str.substr(p0));
    }

    return tokens;
}

int MergerCtx::openInputStreams(const char *file1, const char *file2) 
{
    int ret = 0;

    {
        //m_sei_info1 = probe_sei_info(file1);
        AVFormatContext *ifmt_ctx = nullptr;
        ret = avformat_open_input(&ifmt_ctx, file1, nullptr, nullptr);
        if (ret == 0) {
            AVDictionaryEntry *e = av_dict_get(ifmt_ctx->metadata, "comment", nullptr, AV_DICT_MATCH_CASE);
            if (e != nullptr) {
                std::string value(e->value);
                auto tokens = split(value.c_str(), ',');
                for (auto &clip : tokens) {
                    if(clip.size() < 2)
                        continue;
                    auto duration = split(clip.c_str(), ':');
                    int64_t start = std::stoll(duration.at(0));
                    int64_t end = std::stoll(duration.at(1));
                    m_sei_info1.insert(std::make_pair(start, VideoPadding(VStreamType::LARGE, start, end)));
                    
                }
            }
        }
        if (ifmt_ctx != nullptr) {
            avformat_close_input(&ifmt_ctx);
        }
        m_sei_info2 = probe_sei_info(file2);
    }

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

    std::string  extensions = m_vStream2->getInputFormat()->extensions;
    if (extensions.find("mp4") || extensions.find("mov") || extensions.find("m4a")) {
        if (m_vStream2->getStream(AVMEDIA_TYPE_VIDEO) != nullptr) {
            mp4H264_bsf.reset(initBsfFilter(m_vStream2->getStream(AVMEDIA_TYPE_VIDEO)->codecpar, "h264_mp4toannexb"));
        }
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
        m_frame_duration = av_rescale_q(1, av_inv_q(m_frame_rate), { 1, AV_TIME_BASE });
        m_decodeV.reset(new AVDecoder());
        m_decodeV->cfgCodec(param);
        auto decode_src = m_vStream1->getStream(AVMEDIA_TYPE_VIDEO)->codec;
        auto decode_dst = m_decodeV->getCodecContext(); 
        decode_dst->framerate = m_frame_rate;
        decode_dst->ticks_per_frame = decode_src->ticks_per_frame;
        decode_dst->time_base = { m_vStream1->video_codecParam.frame_rate_den,
            m_vStream1->video_codecParam.frame_rate_num * decode_dst->ticks_per_frame };
        
        if ((ret = m_decodeV->openCodec()) < 0) {
            m_decodeV.reset();
            return ret;
        }
            
    }

    param = m_vStream2->getCodecParameters(AVMEDIA_TYPE_VIDEO);
    if (m_decodeV) {
        m_encodeV.reset(new AVEncoder());
        std::unique_ptr<AVCodecParameters, AVCodecParametersDeleter>
            encode_param(avcodec_parameters_alloc());
        avcodec_parameters_copy(encode_param.get(), param);
        m_encodeV->cfgCodec(encode_param.get(), "libx264");
        if (m_output->getOutputFormat()->flags & AVFMT_GLOBALHEADER)
            m_encodeV->getCodecContext()->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        m_encodeV->getCodecContext()->time_base = m_decodeV->getCodecContext()->time_base;
        m_encodeV->getCodecContext()->ticks_per_frame = m_decodeV->getCodecContext()->ticks_per_frame;

        AVDictionary *options = nullptr;
        av_dict_set(&options, "tune", "film", 0);
        av_dict_set(&options, "crf", "22", 0);
        av_dict_set(&options, "preset", "ultrafast", 0);
        av_dict_set(&options, "x264-params", "aq-mode=1:cabac=1:deblock=-1,-1:subme=1:vbv-maxrate=15000:vbv-bufsize=30000", 0);
        if ((ret = m_encodeV->openCodec(&options)) < 0)
            m_encodeV.reset();
        av_dict_free(&options);
        if (ret < 0)
            return ret;

        ret = filterConf(encode_param.get());
    }

    return ret;
}

int MergerCtx::filterConf(const AVCodecParameters *encode_param) {
    int ret = 0;
    m_filterGraph.reset(new FilterGraph());
    char filter_spec[512] = { 0 };

    std::shared_ptr<FILE> fd = std::shared_ptr<FILE>(std::fopen(Small_LOGO, "r"), [](FILE *ptr) {
        if (ptr != nullptr) { std::fclose(ptr); }
    });

    if (fd) {
        snprintf(filter_spec, sizeof(filter_spec) - 1,
            "movie=%s[wm];[in][wm]overlay=0:0[logo];[logo]scale=w=%d:h=%d:flags=%d[out]",
            Small_LOGO,
            encode_param->width, encode_param->height,
            SWS_FAST_BILINEAR);
    }
    else {
        snprintf(filter_spec, sizeof(filter_spec) - 1,
            "[in]scale=w=%d:h=%d:flags=%d[out]",
            encode_param->width, encode_param->height,
            SWS_FAST_BILINEAR);
    }
    fd = nullptr;

    ret = m_filterGraph->initFilter(m_decodeV->getCodecContext(), m_encodeV->getCodecContext(), filter_spec);
    if (ret < 0) {
        m_filterGraph = nullptr;
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
        }
        if (ret < 0) {
            break;
        }

        //this time_base may change, so use encode timebase;
        AVRational time_base = m_encodeV->getCodecContext()->time_base; 
        //decode small in preview.
        if (packet->data) {
            av_packet_rescale_ts(packet.get(), { 1, AV_TIME_BASE }, time_base);
            ret = decoder->sendPacket(packet.get());
            //get large in preview
            if (ret == 0) {
                packet->stream_index = m_videIndex;
                av_packet_rescale_ts(packet.get(), time_base, { 1, AV_TIME_BASE });
                int64_t pts = packet->pts;
                bool use_large = false;
                for (auto &bitrateInfo : m_sei_info1) {
                    VStreamType type = bitrateInfo.second.streamType;
                    int64_t pts_start = bitrateInfo.second.start_pts - m_frame_duration * Flexible;
                    int64_t pts_end = bitrateInfo.second.end_pts - m_frame_duration * (1 - Flexible);
                    if (pts > pts_start && pts < pts_end) {
                        use_large = type == VStreamType::LARGE ? true : false;
                        break;
                    }
                }
                if (use_large) {
                    if (mp4H264_bsf) {
                        applyBitstream(mp4H264_bsf.get(), packet.get());
                    }
                    insertVideoPacket(packet.release());
                }
            }
            if (ret < 0)
                break;
            ret = decoder->receiveFrame(frame);
            if (ret == AVERROR(EAGAIN)) {
                continue;
            }
            else if (ret < 0) {
                break;
            }
        }
        vp_frame_rescale_ts(frame, time_base, { 1, AV_TIME_BASE });
        break;
    }
    return ret;
}

int MergerCtx::insertVideoPacket(AVPacket *packet){
    //LOGE("insert pts: %lld   dts: %lld  %d\n", packet->pts, packet->dts, packet->flags);
    for (auto iterator = m_packet_queue.cbegin(); iterator != m_packet_queue.cend(); iterator++) {
        if ((*iterator)->dts > packet->dts) {
            m_packet_queue.insert(iterator, std::unique_ptr<AVPacket, AVPacketDeleter>(packet));
            packet = nullptr;
            break;
        };
    }
    if (packet) {
        m_packet_queue.push_back(std::unique_ptr<AVPacket, AVPacketDeleter>(packet));
    }
    return 0;
}

void MergerCtx::writeVideoPacket(AVPacket *packet) {

    int64_t pts = packet->pts;

    bool insert_small = true;
    for (auto &bitrateInfo : m_sei_info1) {
        VStreamType type = bitrateInfo.second.streamType;
        int64_t pts_start = bitrateInfo.second.start_pts - m_frame_duration * Flexible;
        int64_t pts_end = bitrateInfo.second.end_pts - m_frame_duration * (1 - Flexible);
        if (pts > pts_start && pts < pts_end) {
            insert_small = type == VStreamType::LARGE ? false : true;
            break;
        }
    }
    for (auto &bitrateInfo : m_sei_info2) {
        VStreamType type = bitrateInfo.second.streamType;
        int64_t pts_start = bitrateInfo.second.start_pts - m_frame_duration * Flexible;
        int64_t pts_end = bitrateInfo.second.end_pts - m_frame_duration * (1 - Flexible);
        if (pts > pts_start && pts < pts_end) {
            insert_small = type == VStreamType::LARGE ? false : true;
            break;
        }
    }


    if (insert_small) {
        insertVideoPacket(packet);
        packet = nullptr;
    }

    readSlicePacket(pts);
    if (m_packet_queue.size() > 0) {
        auto pkt = m_packet_queue.begin();
        auto pts = (*pkt)->pts;
        //LOGE("write pts: %lld   dts: %lld  %d\n", (*pkt)->pts, (*pkt)->dts, (*pkt)->flags);
        m_output->writePacket(m_packet_queue.front().release());
        m_packet_queue.pop_front();
        if (m_aStream1)
            writeAudioPacket(m_aStream1.get(), m_output.get(), pts, m_audioIndex);
    }
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
            LOGE("filter get frame error\n");
        }
    }
    return ret;
}



int MergerCtx::readSlicePacket(int64_t small_pts) {
    bool use2 = false;
    int ret = 0;
    for (auto bitrateInfo : m_sei_info2) {
        VStreamType type = bitrateInfo.second.streamType;
        int64_t pts_start = bitrateInfo.second.start_pts - m_frame_duration * Flexible;;
        int64_t pts_end = bitrateInfo.second.end_pts - m_frame_duration * (1 - Flexible);
        if (small_pts > pts_start && small_pts < pts_end) {
            use2 = type == VStreamType::LARGE ? true : false;
            break;
        }
    }

    if (use2) {
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        while (true) {
            ret = m_vStream2->readPacket(packet.get());
            if (ret < 0)
                break;
            if (m_vStream2->getStream(packet->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                break;
        }
        if (ret == 0) {
            if (mp4H264_bsf) {
                applyBitstream(mp4H264_bsf.get(), packet.get());
            }
            packet->stream_index = m_videIndex;
            packet->pts += m_slice_offset;
            packet->dts += m_slice_offset;
            insertVideoPacket(packet.release());
        }
    }
    return 0;
}


int MergerCtx::encodeWriteFrame(AVEncoder *encoder, AVFrame *frame) {
    int ret = 0;    
    std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
    
    if (frameScale(frame) < 0)
        return ret;
    vp_frame_rescale_ts(frame, { 1, AV_TIME_BASE }, encoder->getCodecContext()->time_base);

    ret = encoder->sendFrame(frame);
    ret = encoder->receivPacket(packet.get());
    if (ret == AVERROR(EAGAIN)) {
        return ret;
    }
    else if (ret < 0) {
        return ret;
    }
    packet->stream_index = m_videIndex;
    av_packet_rescale_ts(packet.get(), encoder->getCodecContext()->time_base, { 1, AV_TIME_BASE });
    packet->duration = m_frame_duration;
    writeVideoPacket(packet.release());
 
    return 0;
}


void MergerCtx::genCommentInfo(std::map<int64_t, VideoPadding> &sei_info) {

    auto insert_fun = [&sei_info](std::pair<int64_t, VideoPadding>info) {
        if (sei_info.find(info.first) == sei_info.end()) {
            sei_info.insert(std::make_pair(info.second.start_pts,
                VideoPadding(VStreamType::LARGE, info.second.start_pts, info.second.end_pts)));
        }
        else {
            auto exist_info = sei_info.find(info.first)->second;
            if (exist_info.end_pts - exist_info.start_pts < info.second.end_pts - info.second.start_pts) {
                sei_info.insert(std::make_pair(info.second.start_pts,
                    VideoPadding(VStreamType::LARGE, info.second.start_pts, info.second.end_pts)));
            }
        }
    };

    std::for_each(m_sei_info1.begin(), m_sei_info1.end(), insert_fun);
    std::for_each(m_sei_info2.begin(), m_sei_info2.end(), insert_fun);

    std::map<int64_t, VideoPadding> clip_info;
    for (auto info : sei_info) {
        if (clip_info.size() == 0) {
            clip_info.insert(info);
            continue;
        }
        auto clip1 = (--clip_info.end())->second;
        auto clip2 = info.second;
        if (clip1.end_pts < clip2.start_pts) {
            clip_info.insert(info);
        }
        else if (clip1.end_pts < clip2.end_pts) {
            clip_info.erase(clip1.start_pts);
            clip_info.emplace(std::make_pair(clip1.start_pts, VideoPadding(VStreamType::LARGE, clip1.start_pts, clip2.end_pts)));
        }
    
    }

    sei_info = clip_info;
}


void MergerCtx::setCommentInfo() {
    std::map<int64_t, VideoPadding> sei_info;
    genCommentInfo(sei_info);

    char buffer[1024] = { 0 };
    for (auto info : sei_info) {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%lld:%lld,", info.second.start_pts, info.second.end_pts);
    }
    m_output->setMetaDate("comment", buffer);
}

int MergerCtx::mergerLoop() {
    if (m_vStream1 == nullptr ||
        m_vStream2 == nullptr ||
        m_output == nullptr ||
        m_decodeV == nullptr ||
        m_encodeV == nullptr) {
        return -1;
    }

    int ret = 0;

    AVCodecParameters *codecpar = avcodec_parameters_alloc();
    avcodec_parameters_from_context(codecpar, m_encodeV->getCodecContext());
    m_videIndex = m_output->addStream(codecpar);
    avcodec_parameters_free(&codecpar);
    if (m_aStream1)
        m_audioIndex = m_output->addStream(m_aStream1->getCodecParameters(AVMEDIA_TYPE_AUDIO));
    
    setCommentInfo();


    if (m_output->writeHeader() < 0)
        return ret;

    while (ret == 0)
    {
        std::unique_ptr<AVFrame, AVFrameDeleter> frame(av_frame_alloc());
        ret = getVideoFrame(m_vStream1.get(), m_decodeV.get(), frame.get());
        if (ret < 0)
            break;
        frame->pict_type = AV_PICTURE_TYPE_NONE;
        for (auto bitrateInfo : m_sei_info1) {
            VStreamType type = bitrateInfo.second.streamType;
            int64_t pts_start = bitrateInfo.second.start_pts;
            int64_t pts_end = bitrateInfo.second.end_pts;
            if (type == VStreamType::LARGE) {
                if ((frame->pts < pts_start && (frame->pts + frame->pkt_duration) > pts_start) ||
                    (frame->pts < pts_end && (frame->pts + frame->pkt_duration) > pts_end)) {
                    frame->pict_type = AV_PICTURE_TYPE_I;
                };
            }
        }

        for (const auto &bitrateInfo : m_sei_info2) {
            VStreamType type = bitrateInfo.second.streamType;
            const int64_t &pts_start = bitrateInfo.second.start_pts;
            const int64_t &pts_end = bitrateInfo.second.end_pts;

            if (m_slice_offset == 0) {
                if ((frame->pts < pts_start && (frame->pts + frame->pkt_duration) > pts_start)) {
                    if ((frame->pts * 2 + frame->pkt_duration) / 2 < pts_start) {
                        m_slice_offset = (frame->pts + frame->pkt_duration) - pts_start;
                    }
                    else {
                        m_slice_offset = pts_start - frame->pts;
                    }
                    for (auto &bitrateInfo : m_sei_info2) {
                        bitrateInfo.second.start_pts += m_slice_offset;
                        bitrateInfo.second.end_pts += m_slice_offset;
                    }
                    setCommentInfo();
                }
            }

            if (type == VStreamType::LARGE) {
                if ((frame->pts < pts_start && (frame->pts + frame->pkt_duration) > pts_start) ||
                    (frame->pts < pts_end && (frame->pts + frame->pkt_duration) > pts_end)) {
                    frame->pict_type = AV_PICTURE_TYPE_I;
                };
            }
        }

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
        packet->stream_index = m_videIndex;
        av_packet_rescale_ts(packet.get(), m_encodeV->getCodecContext()->time_base, { 1, AV_TIME_BASE });
        packet->duration = m_frame_duration;
        writeVideoPacket(packet.release());
    }
    if (m_aStream1)
        writeAudioPacket(m_aStream1.get(), m_output.get(), AV_NOPTS_VALUE, m_audioIndex);
    ret = m_output->closeOutputForamt();
    return ret;
}
}




using namespace av;
int main(int argc, char *argv[]) {
    int ret = 0;
    av_log_set_level(AV_LOG_WARNING);
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
