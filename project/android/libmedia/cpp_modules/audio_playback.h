#ifndef AUDIO_PLAYBACK_H
#define AUDIO_PLAYBACK_H

#include <thread>
#include <array>
#include <inttypes.h>
#include "oboe/Oboe.h"
#include "audio_resample.h"

class Trace {

public:
    static void beginSection(const char *format, ...);
    static void endSection();
    static void initialize();

private:
    static bool is_tracing_supported_;
};


constexpr int32_t kBufferSizeAutomatic = 0;
constexpr int32_t kMaximumChannelCount = 8;

class AudioPalybackEngine: public oboe::AudioStreamCallback{
public:
    AudioPalybackEngine();
    ~AudioPalybackEngine();

    void setAudioApi(oboe::AudioApi audioApi);

    void setDeviceId(int32_t deviceId);

    void setBufferSizeInBursts(int32_t numBursts);

    double getCurrentOutputLatencyMillis();

    bool isLatencyDetectionSupported();

    void setChannelCount(int channelCount);


    // oboe::StreamCallback methods
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override;
    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override ;
private:
    oboe::AudioApi mAudioApi;
    int32_t mPlaybackDeviceID;
    int32_t mSampleRate;
    oboe::AudioFormat  mAudioFormat;
    int32_t mChannelCount;
    int32_t mFramesPerBurst;    //numbers of sample in one buffer;
    double mCurrentOutputLatencyMillis;
    int32_t mBufferSizeSelection;
    bool mIsLatencyDetectionSupported = false;
    oboe::AudioStream *mPlayStream = nullptr;
    std::unique_ptr<oboe::LatencyTuner> mLatencyTuner;
    std::mutex mRestartingLock;

    std::unique_ptr<AG::AudioResample> m_audio_resample;
    int configAudioResample();


    void checkParam();

    void createPlaybackStream();
    void closeOutputStream();
    void restartStream();
    void setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder);
    oboe::Result calculateCurrentOutputLatencyMillis(oboe::AudioStream *stream, double *latencyMillis);

};


#endif