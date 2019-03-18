package apprtc.org.grafika;

import apprtc.org.grafika.media.AVMediaRecode;
import apprtc.org.grafika.media.AVStruct.MediaInfo;

public interface AVRecodeInterface {
    public static  AVRecodeInterface getRecodeInstance() {
        return new AVMediaRecode();
    }

    public boolean openInputSource(String input);
    public boolean setOutputSourceParm(String clipDirectory, String clipPrefix,
                                       int clipWidth, int clipHeight, int clipBitrate, int clipDurationMs,
                                       int thumbnailWidth, int thumbnailHeight);
    public void setRecodeEventListener(onRecodeEventListener listener);
    public void starRecode();
    public void stopRecode();
    public void waitRecode();
    public static void setLoggCallback(Logging.LoggingCallback callback){
        Logging.setLoggingCallback(callback);
    }



    public static final int ERROR_NONE =                0;
    public static final int ERROR_DECODE =              -0x0010;
    public static final int ERROR_ENCODE =              -0x0020;
    public static final int ERROR_EGL =                 -0x0030;
    public static final int ERROR_BUG =                 -0x0040;


    public interface onRecodeEventListener {
        void onReportMessage(final MediaInfo mediaInfo, final int code, final String message);
        void onOneFileGen(final String fileName);
        void onRecodeFinish();
    }

}
