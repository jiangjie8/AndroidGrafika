package apprtc.org.grafika.media;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;

import apprtc.org.grafika.Logging;
import apprtc.org.grafika.Utils.ThreadUtils;
import apprtc.org.grafika.media.AVStruct.CodecBufferInfo;

public class MediaCodecAudioEncoder implements AVMediaCodec{
    private static final String TAG = "MediaCodecAudioEncoder";
    private Thread mediaCodecThread;
    private MediaCodec mediaCodec;
    final String MIMETYPE_AUDIO = MediaFormat.MIMETYPE_AUDIO_AAC;
    private static final int MEDIA_CODEC_RELEASE_TIMEOUT_MS = 5000; // Timeout for codec releasing.
    private static final int DEQUEUE_TIMEOUT = 0; // Non-blocking, no wait.
    private MediaFormat mediaFormat = null;
    boolean inputFrameEnd = false;
    boolean encodePacketEnd = false;
    private static MediaCodecAudioEncoder runningInstance = null;
    private static MediaCodecAudioEncoderErrorCallback errorCallback = null;
    private static int codecErrors = 0;
    private ByteBuffer configData = null;
    private int droppedFrames;
    private int targetBitrateBps;
    private final MediaCodec.BufferInfo codecBufferInfo = new MediaCodec.BufferInfo();
    public static interface MediaCodecAudioEncoderErrorCallback {
        void onMediaCodecAudioEncoderCriticalError(int codecErrors);
    }

    public static void setErrorCallback(MediaCodecAudioEncoderErrorCallback errorCallback) {
        Logging.d(TAG, "Set error callback");
        MediaCodecAudioEncoder.errorCallback = errorCallback;
    }

    public MediaCodecAudioEncoder() {}

    private void checkOnMediaCodecThread() {
        if (mediaCodecThread.getId() != Thread.currentThread().getId()) {
            throw new RuntimeException("MediaCodecVideoEncoder previously operated on " + mediaCodecThread
                    + " but is now called on " + Thread.currentThread());
        }
    }

    public static void printStackTrace() {
        if (runningInstance != null && runningInstance.mediaCodecThread != null) {
            StackTraceElement[] mediaCodecStackTraces = runningInstance.mediaCodecThread.getStackTrace();
            if (mediaCodecStackTraces.length > 0) {
                Logging.d(TAG, "MediaCodecVideoEncoder stacks trace:");
                for (StackTraceElement stackTrace : mediaCodecStackTraces) {
                    Logging.d(TAG, stackTrace.toString());
                }
            }
        }
    }


    static MediaCodec createEncoderByType(String codecName) {
        try {
            return MediaCodec.createEncoderByType(codecName);
        } catch (IOException e) {
//            e.printStackTrace();
            Log.e(TAG,e.getMessage());
            return null;
        }
    }

    public boolean initEncode(int channels, int sampleRate, int bitDepth, int kbps){
        if (mediaCodecThread != null) {
            throw new RuntimeException("Forgot to release()?");
        }
        runningInstance = this;
        mediaCodecThread = Thread.currentThread();

        targetBitrateBps = 1024*kbps;
        try{
            MediaFormat format = MediaFormat.createAudioFormat(MIMETYPE_AUDIO,sampleRate, channels);
            format.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
            format.setInteger(MediaFormat.KEY_BIT_RATE, targetBitrateBps);
            format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,  1024 * channels * 20);
            format.setInteger(KEY_BIT_WIDTH, bitDepth);

            mediaCodec = createEncoderByType(MIMETYPE_AUDIO);
            if (mediaCodec == null) {
                Logging.e(TAG, "Can not create media encoder");
                release();
                return false;
            }
            mediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            mediaCodec.start();
            droppedFrames = 0;
        }
        catch (IllegalStateException e){
            Logging.e(TAG, "initEncode failed", e);
            release();
            return false;
        }
        return true;
    }



    int dequeueInputBuffer() {
        checkOnMediaCodecThread();
        try {
            return mediaCodec.dequeueInputBuffer(DEQUEUE_TIMEOUT);
        } catch (IllegalStateException e) {
            Logging.e(TAG, "dequeueIntputBuffer failed", e);
            return ERROR_UNKNOW;
        }
    }

    boolean encodeBuffer(int inputBufferIndex, ByteBuffer buffer, int size, long presentationTimestampUs){
        if(inputFrameEnd){
            return true;
        }
        checkOnMediaCodecThread();
        try {
            if(buffer == null){
                Logging.w(TAG, "input frame end");
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                inputFrameEnd = true;
            }else{
                ByteBuffer inputBuffer = mediaCodec.getInputBuffer(inputBufferIndex);
                inputBuffer.position(0);
                inputBuffer.limit(size);
                inputBuffer.put(buffer);
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, size, presentationTimestampUs, 0);
            }
            return true;
        } catch (IllegalStateException e) {
            Logging.e(TAG, "encodeBuffer failed", e);
            return false;
        }
    }

    public boolean sendFrame(ByteBuffer buffer, int size, long presentationTimeStamUs) {
        int index = dequeueInputBuffer();
        if(index < 0){
            Log.w(TAG, "dequeue audio InputBuffer error " + index + " drop frame " + droppedFrames++);
            return false;
        }
        return  encodeBuffer(index, buffer, size, presentationTimeStamUs);
    }


    public CodecBufferInfo flushEncoder(){
        checkOnMediaCodecThread();
        if(!inputFrameEnd){
            sendFrame(null, 0, 0);
        }
        if(!encodePacketEnd){
//            Logging.w(TAG, "flush encode");
            return dequeueOutputBuffer(1000 * 1000);
        }
        return null;
    }

    CodecBufferInfo dequeueOutputBuffer(int dequeueTimeoutUs){
        if(encodePacketEnd){
            return null;
        }
        checkOnMediaCodecThread();
        try {
            if((codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Logging.w(TAG, "encode end, no output packet Available");
                encodePacketEnd = true;
                return null;
            }

            int result = mediaCodec.dequeueOutputBuffer(codecBufferInfo, dequeueTimeoutUs);
            if (result >= 0) {

                if((codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0){
                    Logging.w(TAG, "encode end, no output packet Available");
                    encodePacketEnd = true;
                    releaseOutputBuffer(result);
                    return null;
                }

                boolean isConfigFrame = (codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0;
                if (isConfigFrame) {
                    Logging.d(TAG, "Config frame generated. Offset: " + codecBufferInfo.offset + ". Size: " + codecBufferInfo.size);
                    configData = ByteBuffer.allocateDirect(codecBufferInfo.size);
                    ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(result);
                    outputBuffer.position(codecBufferInfo.offset);
                    outputBuffer.limit(codecBufferInfo.offset + codecBufferInfo.size);
                    configData.put(outputBuffer);

                    mediaCodec.releaseOutputBuffer(result, false);
                    // Query next output.
                    result = mediaCodec.dequeueOutputBuffer(codecBufferInfo, dequeueTimeoutUs);
                }
            }

            if (result >= 0) {
                ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(result);
                outputBuffer.position(codecBufferInfo.offset);
                outputBuffer.limit(codecBufferInfo.offset + codecBufferInfo.size);
                ByteBuffer frameBuffer = ByteBuffer.allocateDirect(codecBufferInfo.size);
                frameBuffer.put(outputBuffer);
                frameBuffer.position(0);
                releaseOutputBuffer(result);
                return new CodecBufferInfo(result, frameBuffer, frameBuffer.limit(), 0, codecBufferInfo.flags,
                        codecBufferInfo.presentationTimeUs, false);
            }else if (result == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                return dequeueOutputBuffer(dequeueTimeoutUs);
            } else if (result == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
//                Logging.w(TAG, "INFO_OUTPUT_FORMAT_CHANGED");
                if(mediaFormat == null){
                    mediaFormat = mediaCodec.getOutputFormat();
                }
                else{
                    Logging.e(TAG, "format change twice");
                }
                return dequeueOutputBuffer(dequeueTimeoutUs);
            } else if (result == MediaCodec.INFO_TRY_AGAIN_LATER) {
                return null;
            }
            throw new RuntimeException("dequeueOutputBuffer: " + result);
        }
        catch (IllegalStateException e) {
            Logging.e(TAG, "dequeueOutputBuffer failed", e);
            return new CodecBufferInfo(-1, null, 0, 0, 0,0,false);
        }
    }

    public CodecBufferInfo receivePacket(){
        return dequeueOutputBuffer(0);
    }

    boolean releaseOutputBuffer(int index) {
        checkOnMediaCodecThread();
        try {
            mediaCodec.releaseOutputBuffer(index, false);
            return true;
        } catch (IllegalStateException e) {
            Logging.e(TAG, "releaseOutputBuffer failed", e);
            return false;
        }
    }

    public MediaFormat getMediaFormat(){
        return mediaFormat;
    }

    public void release() {
        Logging.d(TAG, "Java releaseEncoder");
        checkOnMediaCodecThread();

        class CaughtException {
            Exception e;
        }
        final CaughtException caughtException = new CaughtException();
        boolean stopHung = false;

        if (mediaCodec != null) {
            // Run Mediacodec stop() and release() on separate thread since sometime
            // Mediacodec.stop() may hang.
            final CountDownLatch releaseDone = new CountDownLatch(1);

            Runnable runMediaCodecRelease = new Runnable() {
                @Override
                public void run() {
                    Logging.d(TAG, "Java releaseEncoder on release thread");
                    try {
                        mediaCodec.stop();
                    } catch (Exception e) {
                        Logging.e(TAG, "Media encoder stop failed", e);
                    }
                    try {
                        mediaCodec.release();
                    } catch (Exception e) {
                        Logging.e(TAG, "Media encoder release failed", e);
                        caughtException.e = e;
                    }
                    Logging.d(TAG, "Java releaseEncoder on release thread done");

                    releaseDone.countDown();
                }
            };
            new Thread(runMediaCodecRelease).start();

            if (!ThreadUtils.awaitUninterruptibly(releaseDone, MEDIA_CODEC_RELEASE_TIMEOUT_MS)) {
                Logging.e(TAG, "Media encoder release timeout");
                stopHung = true;
            }

            mediaCodec = null;
        }

        mediaCodecThread = null;

        runningInstance = null;
        droppedFrames = 0;
        if (stopHung) {
            codecErrors++;
            if (errorCallback != null) {
                Logging.e(TAG, "Invoke codec error callback. Errors: " + codecErrors);
                errorCallback.onMediaCodecAudioEncoderCriticalError(codecErrors);
            }
            throw new RuntimeException("Media encoder release timeout.");
        }

        // Re-throw any runtime exception caught inside the other thread. Since this is an invoke, add
        // stack trace for the waiting thread as well.
        if (caughtException.e != null) {
            final RuntimeException runtimeException = new RuntimeException(caughtException.e);
            runtimeException.setStackTrace(ThreadUtils.concatStackTraces(
                    caughtException.e.getStackTrace(), runtimeException.getStackTrace()));
            throw runtimeException;
        }

        Logging.d(TAG, "Java releaseEncoder done");
    }

}
