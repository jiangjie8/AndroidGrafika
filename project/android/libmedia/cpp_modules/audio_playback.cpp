#include "audio_playback.h"
#include "jni_bridge.h"
#include <dlfcn.h>


static const int TRACE_MAX_SECTION_NAME_LENGTH = 100;

// Tracing functions
static void *(*ATrace_beginSection)(const char *sectionName);

static void *(*ATrace_endSection)(void);

typedef void *(*fp_ATrace_beginSection)(const char *sectionName);

typedef void *(*fp_ATrace_endSection)(void);

bool Trace::is_tracing_supported_ = false;

void Trace::beginSection(const char *fmt, ...){

    static char buff[TRACE_MAX_SECTION_NAME_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);

    if (is_tracing_supported_) {
        ATrace_beginSection(buff);
    } else {
        ALOGE("Tracing is either not initialized (call Trace::initialize()) "
                     "or not supported on this device");
    }
}

void Trace::endSection() {

    if (is_tracing_supported_) {
        ATrace_endSection();
    }
}

void Trace::initialize() {

    // Using dlsym allows us to use tracing on API 21+ without needing android/trace.h which wasn't
    // published until API 23
    void *lib = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        ALOGE("Could not open libandroid.so to dynamically load tracing symbols");
    } else {
        ATrace_beginSection =
                reinterpret_cast<fp_ATrace_beginSection >(
                        dlsym(lib, "ATrace_beginSection"));
        ATrace_endSection =
                reinterpret_cast<fp_ATrace_endSection >(
                        dlsym(lib, "ATrace_endSection"));

        if (ATrace_beginSection != nullptr && ATrace_endSection != nullptr){
            is_tracing_supported_ = true;
        }
    }
}


constexpr int64_t kNanosPerMillisecond = 1000000; // Use int64_t to avoid overflows in calculations
constexpr int32_t kDefaultChannelCount = 2; // Stereo


AudioPalybackEngine::AudioPalybackEngine() {
    Trace::initialize();
    mBufferSizeSelection = kBufferSizeAutomatic;

    mSampleRate = 48000;
    mFramesPerBurst = mSampleRate/100;
    mAudioFormat = oboe::AudioFormat::Float;
    mChannelCount = kDefaultChannelCount;
    mAudioApi = oboe::AudioApi::Unspecified;
    mPlaybackDeviceID = oboe::kUnspecified;

    createPlaybackStream();
    ALOGW("AudioPalybackEngine create");
}

AudioPalybackEngine::~AudioPalybackEngine() {
    closeOutputStream();
    ALOGW("AudioPalybackEngine delete");
}

FILE *fd = fopen("/sdcard/Download/music.pcm", "rb");
oboe::DataCallbackResult AudioPalybackEngine::onAudioReady(oboe::AudioStream *audioStream,
                                                           void *audioData, int32_t numFrames){
    int32_t bufferSize = audioStream->getBufferSizeInFrames();
    if(mBufferSizeSelection == kBufferSizeAutomatic){
        mLatencyTuner->tune();
    }
    else if(bufferSize != (mBufferSizeSelection * mFramesPerBurst)){
        audioStream->setBufferSizeInFrames(mBufferSizeSelection * mFramesPerBurst);
        bufferSize = audioStream->getBufferSizeInFrames();
    }

    int32_t underrunCount = audioStream->getXRunCount();
    Trace::beginSection("numFrames %d, Underruns %d, buffer size %d",
                        numFrames, underrunCount, bufferSize);

    int32_t channelCount = audioStream->getChannelCount();
//    ALOGE("getFormat %d, channelCount %d, bufferSize %d,  mBufferSizeSelection %d, mFramesPerBurst %d, numFrames %d",
//          audioStream->getFormat(), channelCount, bufferSize, mBufferSizeSelection, mFramesPerBurst, numFrames);


    int channel_read = 2;
    static uint8_t *buffer = (uint8_t*)malloc(sizeof(int16_t) * channel_read * 480);
    size_t samples_read = fread(buffer, sizeof(int16_t) * channel_read, 480, fd);
    if( samples_read < 480){
        rewind(fd);
    }
    m_audio_resample->pushAudioData((const uint8_t**)&buffer, samples_read);
    m_audio_resample->popAudioData((uint8_t**)&audioData, numFrames);


    if (mIsLatencyDetectionSupported) {
        calculateCurrentOutputLatencyMillis(audioStream, &mCurrentOutputLatencyMillis);
    }

    Trace::endSection();
    return oboe::DataCallbackResult::Continue;
}

void AudioPalybackEngine::onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) {
    ALOGE("callback onErrorAfterClose , error: %s",  oboe::convertToText(error));
    if(error == oboe::Result::ErrorDisconnected){
        restartStream();
    }
}


int AudioPalybackEngine::configAudioResample(){
    m_audio_resample = std::unique_ptr<AG::AudioResample>(new AG::AudioResample());
    m_audio_resample->setInputAudioParm(48000, 2, AG::AAUDIO_FORMAT_PCM_I16);
    m_audio_resample->setOutputAudioParm(mSampleRate, mChannelCount, (enum AG::AudioFormatInner)mAudioFormat);
    return m_audio_resample->createEngine();
}


void AudioPalybackEngine::createPlaybackStream() {
    oboe::AudioStreamBuilder builder;
    setupPlaybackStreamParameters(&builder);
    oboe::Result result = builder.openStream(&mPlayStream);
    if(result == oboe::Result::OK && mPlayStream != nullptr){

        checkParam();
        configAudioResample();
        // Set the buffer size to the burst size - this will give us the minimum possible latency
        mPlayStream->setBufferSizeInFrames(mFramesPerBurst);

        // Create a latency tuner which will automatically tune our buffer size.
        mLatencyTuner = std::unique_ptr<oboe::LatencyTuner>(new oboe::LatencyTuner(*mPlayStream));

        // Start the stream - the dataCallback function will start being called
        result = mPlayStream->requestStart();
        if(result != oboe::Result::OK){
            ALOGE("Error starting stream. %s", oboe::convertToText(result));
        }
        mIsLatencyDetectionSupported = (mPlayStream->getTimestamp(CLOCK_MONOTONIC, 0, 0) !=
                                        oboe::Result::ErrorUnimplemented);

        ALOGW("INFOR: SampleRate %d, FramesPerBurst %d, ChannelCount %d, "
                      "AudioFormat %s, AudioApi %s,  DeviceID %d, LatencyDetectionSupported %d, ",
              mSampleRate, mFramesPerBurst, mChannelCount,
              oboe::convertToText(mAudioFormat), oboe::convertToText(mPlayStream->getAudioApi()), mPlaybackDeviceID,
              mIsLatencyDetectionSupported);
    }
    else{
        ALOGE("Failed to create stream. Error: %s  %p", oboe::convertToText(result), mPlayStream);
    }

}

void AudioPalybackEngine::setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder){
    builder->setAudioApi(mAudioApi);
    builder->setDeviceId(mPlaybackDeviceID);
    builder->setChannelCount(mChannelCount);

    builder->setFormat(mAudioFormat);
    builder->setSampleRate(mSampleRate);
    builder->setDefaultFramesPerBurst(mFramesPerBurst);

    // We request EXCLUSIVE mode since this will give us the lowest possible latency.
    // If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
    builder->setSharingMode(oboe::SharingMode::Exclusive);
    builder->setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder->setCallback(this);
}

void AudioPalybackEngine::checkParam()
{
    if(mPlayStream == nullptr)
        return;

    if(mAudioApi != mPlayStream->getAudioApi()){
        ALOGE("AudioApi  want value %d , true value %d", mAudioApi, mPlayStream->getAudioApi());
    }
    if(mPlaybackDeviceID != mPlayStream->getDeviceId()){
        ALOGE("PlaybackDeviceID  want value %d , true value %d", mPlaybackDeviceID, mPlayStream->getDeviceId());
    }
    if(mChannelCount != mPlayStream->getChannelCount()){
        ALOGE("ChannelCount  want value %d , true value %d", mChannelCount, mPlayStream->getChannelCount());
    }
    if(mAudioFormat != mPlayStream->getFormat()){
        ALOGE("AudioFormat  want value %d , true value %d", mAudioFormat, mPlayStream->getFormat());
    }
    if(mSampleRate != mPlayStream->getSampleRate()){
        ALOGE("SampleRate  want value %d , true value %d", mSampleRate, mPlayStream->getSampleRate());
    }
    if(mFramesPerBurst != mPlayStream->getFramesPerBurst()){
        ALOGE("FramesPerBurst  want value %d , true value %d", mFramesPerBurst, mPlayStream->getFramesPerBurst());
    }
}


void AudioPalybackEngine::closeOutputStream(){
    if(mPlayStream == nullptr)
        return;
    oboe::Result result = oboe::Result::OK;
    if((result = mPlayStream->requestStop()) != oboe::Result::OK){
        ALOGE("Error stopping output stream. %s", oboe::convertToText(result));
    }
    if((result = mPlayStream->close()) != oboe::Result::OK){
        ALOGE("Error closing output stream. %s", oboe::convertToText(result));
    }
}

void AudioPalybackEngine::setAudioApi(oboe::AudioApi audioApi) {
    if (audioApi != mAudioApi) {
        ALOGD("Setting Audio API to %s", oboe::convertToText(audioApi));
        mAudioApi = audioApi;
        restartStream();
    } else {
        ALOGI("Audio API was already set to %s, not setting", oboe::convertToText(audioApi));
    }
}

void AudioPalybackEngine::setDeviceId(int32_t deviceId) {
    if(mPlayStream == nullptr){
        ALOGE("AudioStream is nullptr, the setting parameter is unacceptable");
        return;
    }
    mPlaybackDeviceID = deviceId;
    int32_t currentDeviceId = mPlayStream->getDeviceId();
    if(currentDeviceId != deviceId){
        restartStream();
    }
}


void AudioPalybackEngine::restartStream() {
    ALOGI("Restarting stream");
    if(mRestartingLock.try_lock()){
        closeOutputStream();
        createPlaybackStream();
        mRestartingLock.unlock();
    }
    else{
        ALOGW("Restart stream operation already in progress - ignoring this request");
    }

}

void AudioPalybackEngine::setBufferSizeInBursts(int32_t numBursts) {
    mBufferSizeSelection = numBursts;
}

double AudioPalybackEngine::getCurrentOutputLatencyMillis() {
    return mCurrentOutputLatencyMillis;
}

bool AudioPalybackEngine::isLatencyDetectionSupported() {
    return mIsLatencyDetectionSupported;
}

void AudioPalybackEngine::setChannelCount(int channelCount) {
    if(channelCount != mChannelCount){
        ALOGD("Setting channel count to %d", channelCount);
        mChannelCount = channelCount;
        restartStream();
    }
    else{
        ALOGI("Channel count was already %d, not setting", channelCount);
    }
}

oboe::Result AudioPalybackEngine::calculateCurrentOutputLatencyMillis(oboe::AudioStream *stream,
                                                                      double *latencyMillis) {
    int64_t existingFrameIndex;
    int64_t existingFramePresentationTime;
    oboe::Result result = stream->getTimestamp(CLOCK_MONOTONIC,
                                               &existingFrameIndex,
                                               &existingFramePresentationTime);
    if(result != oboe::Result::OK){
        ALOGE("Error calculating latency: %s", oboe::convertToText(result));
        return result;
    }

    int64_t writeIndex = stream->getFramesWritten();
    int64_t frameIndexDelta = writeIndex - existingFrameIndex;
    int64_t frameTimeDelta = (frameIndexDelta * oboe::kNanosPerSecond) / mSampleRate;
    int64_t nextFramePresentationTime = existingFramePresentationTime + frameTimeDelta;

    using namespace std::chrono;
    int64_t nextFrameWriteTime = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    *latencyMillis = (double) (nextFramePresentationTime - nextFrameWriteTime)
                     / kNanosPerMillisecond;

    return result;
}

