package apprtc.org.grafika;

import android.view.Surface;

import apprtc.org.grafika.media.AVStruct;

public class JNIBridge {

    static {
        System.loadLibrary("avmedia");
        System.loadLibrary("ffmpeg");
    }

    public static native long native_demuxer_createEngine();
    public static native int native_demuxer_openInputFormat(long engineHandle, String inputStr);
    public static native int native_demuxer_closeInputFormat(long engineHandle);
    public static native int native_demuxer_openOutputFormat(long engineHandle, String inputStr, AVStruct.MediaInfo mediaInfo);
    public static native int native_demuxer_closeOutputFormat(long engineHandle);
    public static native int native_demuxer_readPacket(long engineHandle, AVStruct.AVPacket packet);
    public static native int native_demuxer_writePacket(long engineHandle, AVStruct.AVPacket packet);
    public static native int native_demuxer_getMediaInfo(long engineHandle, AVStruct.MediaInfo mediaInfo);


}
