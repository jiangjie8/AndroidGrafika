#include "audio_resample.h"

#include <map>


//#define ALOGD(fmt, ...)   av_log(nullptr, AV_LOG_DEBUG, "jie=[%s:%d]" fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
//#define ALOGI(fmt, ...)   av_log(nullptr, AV_LOG_INFO, "jie=[%s:%d]" fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
//#define ALOGW(fmt, ...)   av_log(nullptr, AV_LOG_WARNING, "jie=[%s:%d]" fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
//#define ALOGE(fmt, ...)   av_log(nullptr, AV_LOG_ERROR, "jie=[%s:%d]" fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace AG {
    std::function<void(AVAudioFifo*)> FiFo_DeleteFun = [](AVAudioFifo *ptr) {
        if (ptr) {
            av_audio_fifo_free(ptr);
        }
    };

    std::function<void(SwrContext*)> Swr_DeleteFun = [](SwrContext *ptr) {
        if (ptr) {
            swr_free(&ptr);
        }
    };

    std::function<void(AVFrame*)> AVFrame_DeleteFun = [](AVFrame *ptr) {
        if (ptr) {
            av_frame_free(&ptr);
        }
    };

    std::map<enum AudioFormatInner, enum AVSampleFormat> AudioResampleMap =
            {{AAUDIO_FORMAT_PCM_I16,   AV_SAMPLE_FMT_S16},
             {AAUDIO_FORMAT_PCM_FLOAT, AV_SAMPLE_FMT_FLT}};

    AudioFiFO::AudioFiFO() :
            m_audioFifo(nullptr, FiFo_DeleteFun) {

    }

    AudioFiFO::~AudioFiFO() {
        m_audioFifo.reset();
    }

    int AudioFiFO::initFiFO(int32_t channels, enum AVSampleFormat format) {
        int result = 0;
        m_channels = channels;
        m_format = format;

        m_audioFifo.reset(av_audio_fifo_alloc(m_format, m_channels, 1));
        if (!m_audioFifo) {
            ALOGE("Could not allocate FIFO");
            result = -1;
            return result;
        }
        return result;
    }

    int AudioFiFO::getFifoSize() {
        return av_audio_fifo_size(m_audioFifo.get());
    }

    void AudioFiFO::refreshFifo() {
        return av_audio_fifo_reset(m_audioFifo.get());
    }

    int AudioFiFO::pushAudioData(uint8_t **input_samples, const int frame_size) {
        int result = 0;
        if ((result = av_audio_fifo_realloc(m_audioFifo.get(), getFifoSize() + frame_size)) < 0) {
            ALOGE("Could not reallocate FIFO");
            return result;
        }

        if (av_audio_fifo_write(m_audioFifo.get(), (void **) input_samples, frame_size) <
            frame_size) {
            ALOGE("Could not write data to FIFO");
            return -1;
        }
        return result;
    }

    int AudioFiFO::popAudioData(uint8_t **output_samples, const int nb_samples) {
        int fifo_size = getFifoSize();
        fifo_size = fifo_size < nb_samples ? fifo_size : nb_samples;
        int result = av_audio_fifo_read(m_audioFifo.get(), (void **) output_samples, fifo_size);
        if (result < fifo_size) {
            ALOGE("av_audio_fifo_read  error  %d < %d", result, fifo_size);
        }
        return result;
    }


    AudioResample::AudioResample() :
            m_swr_ctx(nullptr, Swr_DeleteFun),
            m_audiofifo(new AudioFiFO()),
            m_temp_buffer(nullptr, AVFrame_DeleteFun) {

    }

    AudioResample::~AudioResample() {
        m_swr_ctx.reset();
        m_audiofifo.reset();
    }

    void AudioResample::setInputAudioParm(int32_t sample_rate, int32_t channels,
                                          enum AudioFormatInner format) {
        m_src_samplerate = sample_rate;
        m_src_channels = channels;
        m_src_format = AudioResampleMap[format];
    }

    void AudioResample::setOutputAudioParm(int32_t sample_rate, int32_t channels,
                                           enum AudioFormatInner format) {
        m_dst_samplerate = sample_rate;
        m_dst_channels = channels;
        m_dst_format = AudioResampleMap[format];
    }

    int AudioResample::createEngine() {
        int result = 0;
        m_swr_ctx.reset(swr_alloc());
        if (m_swr_ctx == nullptr) {
            ALOGE("Failed to initialize the resampling context");
            result = -1;
            goto end;
        }
        /* set options */
        av_opt_set_int(m_swr_ctx.get(), "in_channel_layout",
                       av_get_default_channel_layout(m_src_channels), 0);
        av_opt_set_int(m_swr_ctx.get(), "in_sample_rate", m_src_samplerate, 0);
        av_opt_set_sample_fmt(m_swr_ctx.get(), "in_sample_fmt", m_src_format, 0);

        av_opt_set_int(m_swr_ctx.get(), "out_channel_layout",
                       av_get_default_channel_layout(m_dst_channels), 0);
        av_opt_set_int(m_swr_ctx.get(), "out_sample_rate", m_dst_samplerate, 0);
        av_opt_set_sample_fmt(m_swr_ctx.get(), "out_sample_fmt", m_dst_format, 0);

        if ((result = swr_init(m_swr_ctx.get())) < 0) {
            ALOGE("Failed to initialize the resampling context");
            goto end;
        }

        if ((result = m_audiofifo->initFiFO(m_dst_channels, m_dst_format)) < 0) {
            goto end;
        }

        end:
        return result;
    }

    int AudioResample::pushAudioData(const uint8_t **input_samples, const int frame_size) {
        int result;
        reallocFrame(frame_size);
        result = swr_convert(m_swr_ctx.get(), m_temp_buffer->data, m_temp_buffer->nb_samples,
                             input_samples, frame_size);
        if (result < 0) {
            ALOGE("Error while swr_convert  %d < %d", result, frame_size);
            return -1;
        }

        if (m_audiofifo->pushAudioData(m_temp_buffer->data, result) < 0)
            return -1;

        return 0;
    }

/**
* Read data from Fifo.
* @return       number of samples actually read, or negative AVERROR code
*               on failure. The number of samples actually read will not
*               be greater than nb_samples, and will only be less than
*               nb_samples fifo_size is less than nb_samples.
*/
    int AudioResample::popAudioData(uint8_t **output_samples, const int nb_samples) {
        int result = m_audiofifo->popAudioData(output_samples, nb_samples);
        return result;
    }

    int AudioResample::getFifoSize() {
        return m_audiofifo->getFifoSize();
    }

    void AudioResample::refreshFifo() {
        return m_audiofifo->refreshFifo();
    }

    void AudioResample::reallocFrame(int frame_size) {
        if (!m_temp_buffer) {
            m_temp_buffer.reset(av_frame_alloc(), AVFrame_DeleteFun);
        }
        if (m_temp_buffer && m_temp_buffer->nb_samples < frame_size) {
            av_frame_unref(m_temp_buffer.get());
            m_temp_buffer->format = m_dst_format;
            m_temp_buffer->nb_samples = av_rescale_rnd(
                    swr_get_delay(m_swr_ctx.get(), m_src_samplerate) + frame_size,
                    m_dst_samplerate, m_src_samplerate, AV_ROUND_UP);
            m_temp_buffer->sample_rate = m_dst_samplerate;
            m_temp_buffer->channel_layout = av_get_default_channel_layout(m_dst_channels);
            av_frame_get_buffer(m_temp_buffer.get(), 32);
        }
    }


}



//int main(int argc, char *argv) {
//    FILE *fd = fopen(R"(D:\videoFile\music.pcm)", "rb");
//    AudioResample ar;
//    ar.setInputAudioParm(48000, 2, AV_SAMPLE_FMT_S16);
//    ar.setOutputAudioParm(96000, 4, AV_SAMPLE_FMT_S32);
//    ar.createEngine();
//    AVFrame *frame = av_frame_alloc();
//    frame->format = AV_SAMPLE_FMT_S16;
//    frame->nb_samples = 480;
//    frame->channel_layout = av_get_default_channel_layout(2);
//    frame->sample_rate = 48000;
//    av_frame_get_buffer(frame, 32);
//
//
//    int out_samples = 5000;
//    int perFrame_size = sizeof(int32_t) * 4;
//    uint8_t *outbuffer = (uint8_t*)malloc(perFrame_size* out_samples);
//    for (;;) {
//        int ret = fread(frame->data[0], sizeof(int16_t) * 2, frame->nb_samples, fd);
//        if (ret < frame->nb_samples)
//            break;
//        ret = ar.pushAudioData((const uint8_t**)frame->data, frame->nb_samples);
//
//        ret = ar.popAudioData(&outbuffer, out_samples);
//
//        static FILE *fd_out = fopen(R"(C:\Users\JIE\aaaa.pcm)", "wb");
//        fwrite(outbuffer, perFrame_size, ret, fd_out);
//
//
//    }
//
//
//}

