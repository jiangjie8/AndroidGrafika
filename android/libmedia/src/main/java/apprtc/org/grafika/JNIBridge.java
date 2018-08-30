package apprtc.org.grafika;

import android.view.Surface;

import apprtc.org.grafika.media.AVStruct;

public class JNIBridge {

    static {
        System.loadLibrary("avmedia");
        System.loadLibrary("ffmpeg");
    }


    public static void createCodec_MediaCodec(){
        bStartCodec = false;
        mMediaCodecEngineHandle = native_createEngine_mediacodec();
    }

    public static Surface configureCodec_MediaCodec(int width, int height, boolean useSurface){
        return native_configureCodec_mediacodec(mMediaCodecEngineHandle, width, height, useSurface);
    }

    public static void startCodec_MediaCodec(){
        native_startCodec_mediacodec(mMediaCodecEngineHandle);
        bStartCodec = true;
    }

    public static void stopCodec_MediaCodec(){
        bStartCodec = false;
        native_stopCodec_mediacodec(mMediaCodecEngineHandle);
    }

    public static void deleteCodec_MediaCodec(){
        bStartCodec = false;
        native_deleteEngine_mediacodec(mMediaCodecEngineHandle);
    }
    public static boolean bStartCodec = false;
    private static long mMediaCodecEngineHandle = 0;

    // call MediaCodec
    private static native long native_createEngine_mediacodec();
    private static native Surface native_configureCodec_mediacodec(long engineHandle, int width, int height, boolean useSurface);
    private static native void native_startCodec_mediacodec(long engineHandle);
    private static native void native_stopCodec_mediacodec(long engineHandle);
    private static native void native_deleteEngine_mediacodec(long engineHandle);





    // Native methods
    public static native long native_createEngine_audioOboe();
    public static native void native_deleteEngine_audioOboe(long engineHandle);
    public static native void native_setToneOn_audioOboe(long engineHandle, boolean isToneOn);
    public static native void native_setAudioApi_audioOboe(long engineHandle, int audioApi);
    public static native void native_setAudioDeviceId_audioOboe(long engineHandle, int deviceId);
    public static native void native_setChannelCount_audioOboe(long mEngineHandle, int channelCount);
    public static native void native_setBufferSizeInBursts_audioOboe(long engineHandle, int bufferSizeInBursts);
    public static native double native_getCurrentOutputLatencyMillis_audioOboe(long engineHandle);
    public static native boolean native_isLatencyDetectionSupported_audioOboe(long engineHandle);


    public static native long native_demuxer_createEngine();
    public static native int native_demuxer_openInputFormat(long engineHandle, String inputStr);
    public static native int native_demuxer_closeInputFormat(long engineHandle);
    public static native int native_demuxer_openOutputFormat(long engineHandle, String inputStr);
    public static native int native_demuxer_closeOutputFormat(long engineHandle);
    public static native int native_demuxer_readPacket(long engineHandle, AVStruct.AVPacket packet);
    public static native int native_demuxer_writePacket(long engineHandle, AVStruct.AVPacket packet);
    public static native int native_demuxer_getMediaInfo(long engineHandle, AVStruct.MediaInfo mediaInfo);


}
