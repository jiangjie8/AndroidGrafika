package apprtc.org.grafika.media;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.SystemClock;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import apprtc.org.grafika.Logging;
import apprtc.org.grafika.Utils.ThreadUtils;
import apprtc.org.grafika.media.AVStruct.CodecBufferInfo;

public class MediaCodecAudioDecoder implements AVMediaCodec {
    private static final String TAG = "MediaCodecAudioDecoder";
    private static final int MEDIA_CODEC_RELEASE_TIMEOUT_MS = 5000;
    private static final int DEQUEUE_TIMEOUT = 0;
    private Thread mediaCodecThread;
    private MediaCodec mediaCodec;

    private static MediaCodecAudioDecoder runningInstance = null;
    private boolean inputPacketEnd = false;
    private boolean decodeFrameEnd = false;
    private final MediaCodec.BufferInfo codecBufferInfo = new MediaCodec.BufferInfo();

    private static MediaCodecAudioDecoderErrorCallback errorCallback = null;
    private static int codecErrors = 0;

    public static interface MediaCodecAudioDecoderErrorCallback {
        void onMediaCodecAudioDecoderCriticalError(int codecErrors);
    }

    public static void setErrorCallback(MediaCodecAudioDecoderErrorCallback errorCallback) {
        Logging.d(TAG, "Set error callback");
        MediaCodecAudioDecoder.errorCallback = errorCallback;
    }

    public MediaCodecAudioDecoder() {}

    private void checkOnMediaCodecThread() throws IllegalStateException {
        if (mediaCodecThread.getId() != Thread.currentThread().getId()) {
            throw new IllegalStateException("MediaCodecAudioDecoder previously operated on "
                    + mediaCodecThread + " but is now called on " + Thread.currentThread());
        }
    }

    public boolean initDecode(MediaFormat format)
    {
        if (mediaCodecThread != null) {
            throw new RuntimeException("initDecode: Forgot to release()?");
        }
        runningInstance = this;
        mediaCodecThread = Thread.currentThread();

        try {
            mediaCodec = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME));
            if (mediaCodec == null) {
                Logging.e(TAG, "Can not create media decoder");
                return false;
            }

            mediaCodec.configure(format, null, null, 0);
            mediaCodec.start();


            return true;
        }catch (IllegalStateException e) {
            Logging.e(TAG, "initDecode failed", e);
            return false;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }



    public void release() {

        checkOnMediaCodecThread();

        // Run Mediacodec stop() and release() on separate thread since sometime
        // Mediacodec.stop() may hang.
        final CountDownLatch releaseDone = new CountDownLatch(1);

        Runnable runMediaCodecRelease = new Runnable() {
            @Override
            public void run() {
                try {
                    Logging.d(TAG, "Java releaseDecoder on release thread");
                    mediaCodec.stop();
                    mediaCodec.release();
                    Logging.d(TAG, "Java releaseDecoder on release thread done");
                } catch (Exception e) {
                    Logging.e(TAG, "Media decoder release failed", e);
                }
                releaseDone.countDown();
            }
        };
        new Thread(runMediaCodecRelease).start();

        if (!ThreadUtils.awaitUninterruptibly(releaseDone, MEDIA_CODEC_RELEASE_TIMEOUT_MS)) {
            Logging.e(TAG, "Media decoder release timeout");
            codecErrors++;
            if (errorCallback != null) {
                Logging.e(TAG, "Invoke codec error callback. Errors: " + codecErrors);
                errorCallback.onMediaCodecAudioDecoderCriticalError(codecErrors);
            }
        }

        mediaCodec = null;
        mediaCodecThread = null;
        runningInstance = null;

        Logging.d(TAG, "Java releaseDecoder done");
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

    boolean queueInputBuffer(ByteBuffer buffer, int inputBufferIndex, int size, long presentationTimeStamUs) {
        checkOnMediaCodecThread();
        if(inputPacketEnd){
            return true;
        }
        try {
            if(buffer == null){
                Logging.i(TAG, "input packet end");
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                inputPacketEnd = true;
            }
            else{
                ByteBuffer inputBuffer = mediaCodec.getInputBuffer(inputBufferIndex);
                inputBuffer.position(0);
                inputBuffer.limit(size);
                inputBuffer.put(buffer);
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, size, presentationTimeStamUs, 0);
            }
            return true;
        } catch (IllegalStateException e) {
            Logging.e(TAG, "decode failed", e);
            return false;
        }
    }

    public int sendPacket(ByteBuffer buffer, int size, long presentationTimeStamUs){
        int ret = 0;
        int inputBufferIndex = dequeueInputBuffer();
        if(inputBufferIndex >= 0){
            if(queueInputBuffer(buffer, inputBufferIndex, size, presentationTimeStamUs))
                ret = 0;
            else
                ret = ERROR_UNKNOW;
        }
        else if(inputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER){
            ret = ERROR_TRY_AGAIN;
        }
        else{
            ret = ERROR_UNKNOW;
        }
        return ret;
    }

    public CodecBufferInfo receiveFrame(){
        return dequeueOutputBuffer(0);
    }

    public CodecBufferInfo flushDecoder(){
        checkOnMediaCodecThread();
        if(!inputPacketEnd){
            sendPacket(null, 0, 0);
        }
        if(!decodeFrameEnd){
//            Logging.i(TAG, "flush decode");
            return dequeueOutputBuffer(1000 * 1000);
        }
        return null;
    }

    private CodecBufferInfo dequeueOutputBuffer(int dequeueTimeoutUs) {
        checkOnMediaCodecThread();
        // Drain the decoder until receiving a decoded buffer or hitting
        // MediaCodec.INFO_TRY_AGAIN_LATER.
        if(decodeFrameEnd){
            return null;
        }
        try{
            final int result = mediaCodec.dequeueOutputBuffer(codecBufferInfo, dequeueTimeoutUs);
            if(result >= 0){
                if((codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0){
                    decodeFrameEnd = true;
                    Logging.i(TAG, "decode end, no frame will be available after this");
                }
                ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(result);
                outputBuffer.position(codecBufferInfo.offset);
                outputBuffer.limit(codecBufferInfo.offset + codecBufferInfo.size);
                ByteBuffer buffer = ByteBuffer.allocateDirect(codecBufferInfo.size);
                buffer.put(outputBuffer);
                buffer.rewind();
                releaseOutputBuffer(result);
                return new CodecBufferInfo(result, buffer, codecBufferInfo.size, codecBufferInfo.presentationTimeUs);
            }
            else if(result == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED){
                return dequeueOutputBuffer(dequeueTimeoutUs);
            }
            else if(result == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                mediaCodec.getOutputFormat();
//                Logging.i(TAG, "INFO_OUTPUT_FORMAT_CHANGED");
                return dequeueOutputBuffer(dequeueTimeoutUs);
            }
            else if(result == MediaCodec.INFO_TRY_AGAIN_LATER){
                return null;
            }
            throw new RuntimeException("dequeueOutputBuffer: " + result);
        }
        catch (IllegalStateException e){
            Logging.e(TAG, "dequeueOutputBuffer failed", e);
            return null;
        }
    }

    void releaseOutputBuffer(int index)
            throws IllegalStateException, MediaCodec.CodecException {
        checkOnMediaCodecThread();
        mediaCodec.releaseOutputBuffer(index, false /* render */);
    }


}















