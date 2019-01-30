#include "core/media_common/media_struct.h"
#include "ff_recode.h"

namespace av{
    static void av_log_default_callback(void* ptr, int level, const char* fmt, va_list vl) {
        if(level > av_log_get_level())
            return;
        char buffer[1024] = {0};
        switch(level){
            case AV_LOG_PANIC:{}
            case AV_LOG_FATAL:{}
            case AV_LOG_ERROR:{
                vsnprintf(buffer, sizeof(buffer) -1, fmt, vl);
                ALOGE("%s", buffer);
                break;
            }
            case AV_LOG_WARNING:{
                vsnprintf(buffer, sizeof(buffer) -1, fmt, vl);
                ALOGW("%s", buffer);
                break;
            }
            case AV_LOG_INFO:{
                vsnprintf(buffer, sizeof(buffer) -1, fmt, vl);
                ALOGI("%s", buffer);
                break;
            }
            case AV_LOG_DEBUG:{
                vsnprintf(buffer, sizeof(buffer) -1, fmt, vl);
                ALOGD("%s", buffer);
                break;
            }
            default:{
                vsnprintf(buffer, sizeof(buffer) -1, fmt, vl);
                ALOGV("%s", buffer);
                break;
            }
        }
    }
    constexpr int AUDIO_BITRATE = 48 * 1024;
    static void initOnce(JNIEnv *env){
        static bool init = false;
        if(!init){
            init = true;
            av_log_set_callback(av_log_default_callback);
        }
    }

    int FFRecoder::initH264Bsf(const AVCodecParameters *codecpar){
        std::string  extensions = m_inputFormat->getInputFormat()->extensions;
        if(extensions.find("mp4")
           || extensions.find("mov")
           || extensions.find("m4a")){

        }
        else{
            return -1;
        }

        char *bitstream_filter = NULL;
        if(codecpar->codec_id == AV_CODEC_ID_H264){
            bitstream_filter = "h264_mp4toannexb";
        }
        else if(codecpar->codec_id == AV_CODEC_ID_H265){
            bitstream_filter = "hevc_mp4toannexb";
        }
        else{
            return -1;
        }

        m_mp4H264_bsf.reset(initBsfFilter(codecpar, bitstream_filter));

        return 0;
    }


    int FFRecoder::openInputFormat(JNIEnv *env, const char *inputStr) {
        int ret = 0;
        initOnce(env);
        m_input = inputStr;
        m_inputFormat.reset(new AVDemuxer());
        ret = m_inputFormat->openInputFormat(m_input.c_str());
        if(ret < 0)
            return ret;
        m_video_codecParam = m_inputFormat->video_codecParam;
        m_audio_codecParam = m_inputFormat->audio_codecParam;
        m_stream_info = m_inputFormat->stream_info;

        m_audioStream.reset(new AVDemuxer());
        ret = m_audioStream->openInputFormat(m_input.c_str());
        if(ret <0 || m_audioStream->getStream(AVMEDIA_TYPE_AUDIO) == nullptr){
            m_audioStream.reset();
        }

        initH264Bsf(m_inputFormat->getStream(AVMEDIA_TYPE_VIDEO)->codecpar);

        const AVCodecParameters *codecParameters = m_inputFormat->getCodecParameters(AVMEDIA_TYPE_AUDIO);
        if(codecParameters != nullptr){
            m_audio_decoder.reset(new AVDecoder());
            if(m_audio_decoder->cfgCodec(codecParameters) < 0)
                m_audio_decoder.reset();
            m_audio_decoder->openCodec();

            std::unique_ptr<AVCodecParameters, AVCodecParametersDeleter> encoderParameters(avcodec_parameters_alloc());
            avcodec_parameters_copy(encoderParameters.get(), codecParameters);
            encoderParameters->codec_id = AV_CODEC_ID_AAC;
            encoderParameters->bit_rate = AUDIO_BITRATE;
            m_audio_encoder.reset(new AVEncoder());
            if(m_audio_encoder->cfgCodec(encoderParameters.get(), "aac") < 0)
                m_audio_encoder.reset();
            m_audio_encoder->openCodec();
            mFilterGraph.initFilter(m_audio_decoder->getCodecContext(), m_audio_encoder->getCodecContext(), "anull");
        }



        return ret;
    }


    int FFRecoder::getMediaInfo(JNIEnv *env, jobject mediaInfo) {

        J4A_MediaInfo_setFiled_bitrate(env, mediaInfo, m_stream_info.bitrate);
        J4A_MediaInfo_setFiled_duration(env, mediaInfo, m_stream_info.duration);
        J4A_MediaInfo_setFiled_startTime(env, mediaInfo, m_stream_info.start_time);
        if(m_video_codecParam.width > 0 &&
           m_video_codecParam.height > 0){
            J4A_MediaInfo_setFiled_videoCodecID(env, mediaInfo, m_video_codecParam.codec_id);
            J4A_MediaInfo_setFiled_width(env, mediaInfo, m_video_codecParam.width);
            J4A_MediaInfo_setFiled_height(env, mediaInfo, m_video_codecParam.height);
            int framerate = 25;
            if(m_video_codecParam.frame_rate_num > 0 && m_video_codecParam.frame_rate_den > 0){
                framerate = m_video_codecParam.frame_rate_num/m_video_codecParam.frame_rate_den;
            }
            J4A_MediaInfo_setFiled_framerate(env, mediaInfo, framerate);
        }

        if(m_audio_codecParam.sample_rate > 0 &&
           m_audio_codecParam.channels > 0){
            J4A_MediaInfo_setFiled_audioCodecID(env, mediaInfo, m_audio_codecParam.codec_id);
            J4A_MediaInfo_setFiled_channels(env, mediaInfo, m_audio_codecParam.channels);
            J4A_MediaInfo_setFiled_sampleRate(env, mediaInfo, m_audio_codecParam.sample_rate);
            J4A_MediaInfo_setFiled_sampleDepth(env, mediaInfo, m_audio_codecParam.sample_depth);
            J4A_MediaInfo_setFiled_frameSize(env, mediaInfo, m_audio_codecParam.frame_size);
        }
        return 0;
    }


    int FFRecoder::openOutputFormat(JNIEnv *env, const char *outputStr, jobject mediaInfo){
        int ret = 0;
        m_timestamp_start = AV_NOPTS_VALUE;
        m_muxer.reset(new AVMuxer());
        m_muxer->openOutputFormat(outputStr);

        if(m_video_codecParam.width > 0 &&
           m_video_codecParam.height > 0){
            const AVCodecParameters *src_codecpar = m_inputFormat->getCodecParameters(AVMEDIA_TYPE_VIDEO);
            AVCodecParameters *codecpar =  avcodec_parameters_alloc();
            codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
            codecpar->width = J4A_MediaInfo_getFiled_width(env, mediaInfo);
            codecpar->height = J4A_MediaInfo_getFiled_height(env, mediaInfo);
            codecpar->sample_aspect_ratio = src_codecpar->sample_aspect_ratio;
            codecpar->codec_id = (enum AVCodecID)J4A_MediaInfo_getFiled_videoCodecID(env, mediaInfo);
            codecpar->format = AV_PIX_FMT_YUV420P;
            v_index = m_muxer->addStream(codecpar);
            avcodec_parameters_free(&codecpar);
        }

        if(m_audio_codecParam.sample_rate > 0 &&
           m_audio_codecParam.channels > 0 && m_audio_encoder != nullptr){
            AVCodecParameters *codecpar =  avcodec_parameters_alloc();
            avcodec_parameters_from_context(codecpar, m_audio_encoder->getCodecContext());
            a_index = m_muxer->addStream(codecpar);
            avcodec_parameters_free(&codecpar);
        }
        if(m_spspps_buffer){
            m_muxer->setExternalData(m_spspps_buffer, m_spspps_buffer_size, v_index);
        }
        ret = m_muxer->writeHeader();
        if(ret < 0)
            m_muxer.reset();
        return ret;

    }

    int FFRecoder::closeOutputFormat(){
        int ret = -1;
        if(m_muxer)
            ret = m_muxer->closeOutputForamt();
        m_muxer.reset();
        return ret;
    }


    int FFRecoder::readVideoPacket(JNIEnv *env, jobject packet_obj) {
        int ret = 0;
        std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
        while(true){
//            av_packet_unref(packet.get());
//            av_init_packet(packet.get());
            ret = m_inputFormat->readPacket(packet.get());
            if(ret == AVERROR_EOF){
                writeAudioPacket(AV_NOPTS_VALUE);
                return AVERROR_EOF;
            }
            else if(ret < 0){
                return ret;
            }
            int stream_index = packet->stream_index;
            int mediaType = m_inputFormat->getStream(stream_index)->codecpar->codec_type;
            if(mediaType == AVMEDIA_TYPE_VIDEO)
                break;

        }

        applyBitstream(m_mp4H264_bsf.get(), packet.get());

        if (J4A_set_avpacket(env, packet_obj, packet->data, packet->size, packet->pts,
                             packet->dts, AVMEDIA_TYPE_VIDEO) < 0) {
            ALOGE("GetDirectBufferAddress error");
            return -1;
        }

        return ret;
    }
    int FFRecoder::writeInterleavedPacket(AVPacket *packet){
        if(m_timestamp_start == AV_NOPTS_VALUE){
            m_timestamp_start = packet->pts;
        }
        packet->pts -= m_timestamp_start;
        packet->dts -= m_timestamp_start;
//        ALOGI("write stream index %d, pts %f dts %f  %f\n", packet->stream_index, packet->pts/1000000.0, packet->dts/1000000.0, packet->duration/1000000.0);
        return m_muxer->writePacket(packet);
    }



    int FFRecoder::writePacket(JNIEnv *env, jobject packet_obj) {
        int ret = 0;
        int64_t pts  = J4A_AVPacket_getFiled_ptsUs(env, packet_obj);
        int64_t dts  = J4A_AVPacket_getFiled_dtsUs(env, packet_obj);

        jobject bytebuffer = J4A_AVPacket_getFiled_buffer(env, packet_obj);
        void    *buf_ptr  = env->GetDirectBufferAddress(bytebuffer);
        int     buffer_size = J4A_AVPacket_getFiled_bufferSize(env, packet_obj);
        int     buffer_offset = J4A_AVPacket_getFiled_bufferOffset(env, packet_obj);
        int     buffer_flags = J4A_AVPacket_getFiled_bufferFlags(env, packet_obj);
        std::unique_ptr<AVPacket, AVPacketDeleter> vpacket(av_packet_alloc());
        av_init_packet(vpacket.get());
        av_new_packet(vpacket.get(), buffer_size);
        vpacket->stream_index = v_index;
        vpacket->pts = pts;
        vpacket->dts = dts;
        vpacket->duration = av_rescale_q(1 , av_inv_q({m_video_codecParam.frame_rate_num, m_video_codecParam.frame_rate_den}), AV_TIME_BASE_Q);
        vpacket->flags = buffer_flags;
        memcpy(vpacket->data, buf_ptr, buffer_size);
        if(m_spspps_buffer == nullptr &&  (buffer_flags & BUFFER_FLAG_EXTERNAL_DATA) != 0){
            m_spspps_buffer_size = buffer_size;
            m_spspps_buffer = (uint8_t *)av_malloc(m_spspps_buffer_size);
            memcpy(m_spspps_buffer, vpacket->data, m_spspps_buffer_size);
            m_muxer->setExternalData(m_spspps_buffer, m_spspps_buffer_size, v_index);
            J4A_DeleteLocalRef__p(env, &bytebuffer);
            return 0;
        }

        ret = writeInterleavedPacket(vpacket.get());
        if(m_previous_apts == AV_NOPTS_VALUE || m_previous_apts < pts){
            int ret_a = writeAudioPacket(pts);
            if(ret_a != AVERROR_EOF && ret_a < 0)
                ALOGE("write audio error  ret %d\n", ret_a);
        }
        J4A_DeleteLocalRef__p(env, &bytebuffer);
        return ret;
    }
    int FFRecoder::filterAudioFrame(AVFrame *frame){
        int ret = 0;
        ret = mFilterGraph.addFrame(frame);
        if (ret < 0) {
            ALOGE("Error while feeding the filtergraph\n");
            return ret;
        }

        av_frame_unref(frame);
        ret = mFilterGraph.getFrame(frame);
        if (ret < 0) {
            ALOGE("Error while  get filtergraph\n");
            return ret;
        }
        return ret;
    }
    int FFRecoder::getAudioFrame(AVFrame *frame){
        if(m_audioStream == nullptr)
            return -1;
        int ret = 0;
        while(true){
            std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
            while (true) {
//                av_packet_unref(packet.get());
//                av_init_packet(packet.get());
                ret = m_audioStream->readPacket(packet.get());
                if(ret == AVERROR_EOF){
                    break;
                }
                else if(ret < 0){
                    return ret;
                }
                int stream_index = packet->stream_index;
                int mediaType = m_audioStream->getStream(stream_index)->codecpar->codec_type;
                if(mediaType == AVMEDIA_TYPE_AUDIO)
                    break;
            }

            if(ret == AVERROR_EOF){
                ret = m_audio_decoder->flushCodec(frame);
                if(ret == 0){
                    ret = filterAudioFrame(frame);
                    frame->pts = av_rescale_q(frame->pts, m_audio_decoder->getCodecContext()->time_base, { 1, AV_TIME_BASE });
                }
                return  ret;
            }
            else if(ret < 0){
                break;
            }

            int stream_index = packet->stream_index;
            av_packet_rescale_ts(packet.get(), AV_TIME_BASE_Q, m_audio_decoder->getCodecContext()->time_base);
            ret = m_audio_decoder->sendPacket(packet.get());
            if(ret == AVERROR(EAGAIN)){

            }
            else if(ret < 0){
                ALOGE("avcodec_send_packet error");
                break;
            }
            ret = m_audio_decoder->receiveFrame(frame);
            if(ret == AVERROR(EAGAIN)){
                continue;
            }
            else if(ret < 0){
                break;
            }
            ret = filterAudioFrame(frame);
            frame->pts = av_rescale_q(frame->pts, m_audio_decoder->getCodecContext()->time_base, { 1, AV_TIME_BASE });
            return ret;
        }

    }

    int FFRecoder::writeAudioPacket(int64_t video_pts){
        if(m_audioStream == nullptr)
            return 0;
        int ret = 0;
        while(true ){
            std::unique_ptr<AVFrame, AVFrameDeleter> aframe(av_frame_alloc());
            ret = getAudioFrame(aframe.get());
            if(ret == AVERROR_EOF){

            }
            else if(ret < 0){
                break;
            }

            std::unique_ptr<AVPacket, AVPacketDeleter> packet(av_packet_alloc());
            aframe->pts = av_rescale_q(aframe->pts, { 1, AV_TIME_BASE }, m_audio_encoder->getCodecContext()->time_base);
            ret = drainAudioEncoder(ret == AVERROR_EOF? nullptr:aframe.get(), packet.get());
            if(ret == AVERROR(EAGAIN)){
                continue;
            }
            else if(ret == AVERROR_EOF){
                break;
            }
            else if(ret < 0){
                break;
            }

            packet->stream_index = a_index;
            av_packet_rescale_ts(packet.get(), m_audio_encoder->getCodecContext()->time_base, AV_TIME_BASE_Q);
            int64_t audio_pts = packet->pts;
            int64_t audio_duration = packet->duration;
            m_previous_apts = audio_pts;
            if((ret = writeInterleavedPacket(packet.get())) < 0){
                break;
            }

            if(audio_pts + audio_duration > video_pts &&  video_pts != AV_NOPTS_VALUE){
                break;
            }
        }
        return ret;
    }



    int FFRecoder::drainAudioEncoder(const AVFrame *frame, AVPacket *packet){
        if(m_audio_encoder == nullptr)
            return AVERROR_EOF;
        int ret = 0;
        if(frame != nullptr){
            ret = m_audio_encoder->sendFrame(frame);
            if(ret == AVERROR(EAGAIN)){

            }
            else if(ret < 0){
                ALOGE("avcodec_send_frame error");
                return ret;
            }
            ret = m_audio_encoder->receivPacket(packet);
        }
        else{
            ret = m_audio_encoder->flushCodec(packet);
        }

        return ret;
    }
}