package apprtc.org.grafika;

import android.os.Handler;

import apprtc.org.grafika.media.AVMediaRecode;

public interface AVRecodeInterface {
    public static  AVRecodeInterface getRecodeInstance() {
        return new AVMediaRecode();
    }

    public boolean openInputSource(String input);
    public boolean setOutputSourceParm(String clipDirectory, String clipPrefix,
                            int clipWidth, int clipHeight, int clipBitrate, int clipDurationMs);
    public void setRecodeEventListener(onRecodeEventListener listener);
    public void starRecode();
    public void stopRecode();
    public void waitRecode();
    public static void setLoggCallback(Logging.LoggingCallback callback){
        Logging.setLoggingCallback(callback);
    }

    public interface onRecodeEventListener {
        void onPrintReport(final String message);
        void onErrorMessage(final int errorCode, final String errorMessage);
        void onOneFileGen(final String fileName);
        void onRecodeFinish();
    }

}
