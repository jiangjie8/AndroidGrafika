package apprtc.org.grafika.media;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.opengl.GLES20;
import android.util.Log;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import apprtc.org.grafika.Logging;
import apprtc.org.grafika.Utils.ThreadUtils;
import apprtc.org.grafika.gles.EglBase;
import apprtc.org.grafika.gles.EglBase14;
import apprtc.org.grafika.gles.GlRectDrawer;
import apprtc.org.grafika.media.AVStruct.CodecBufferInfo;


public class MediaCodecVideoEncoder implements AVMediaCodec{
    private static final String TAG = "MediaCodecVideoEncoder";
//    final String MIMETYPE_VIDEO = MediaFormat.MIMETYPE_VIDEO_HEVC;
    private static final int VIDEO_ControlRateConstant = MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR;
    private static final int FRAME_INTERVAL = 5;
    public enum VideoCodecType {
        VIDEO_CODEC_VP8,
        VIDEO_CODEC_VP9,
        VIDEO_CODEC_H264;
    }

    public static interface MediaCodecVideoEncoderErrorCallback {
        void onMediaCodecVideoEncoderCriticalError(int codecErrors);
    }

    public static void setErrorCallback(MediaCodecVideoEncoderErrorCallback errorCallback) {
        Logging.d(TAG, "Set error callback");
        MediaCodecVideoEncoder.errorCallback = errorCallback;
    }
    private static final int MEDIA_CODEC_RELEASE_TIMEOUT_MS = 5000; // Timeout for codec releasing.
    private static final int DEQUEUE_TIMEOUT = 0; // Non-blocking, no wait.
    private static MediaCodecVideoEncoder runningInstance = null;
    private static MediaCodecVideoEncoderErrorCallback errorCallback = null;
    private Thread mediaCodecThread;
    private MediaCodec mediaCodec;
    private EglBase14 eglBase;
    private int width;
    private int height;
    private Surface inputSurface;
    private GlRectDrawer drawer;
    private MediaFormat mediaFormat = null;
    private int targetBitrateBps;
    private int targetFps;
    private static int codecErrors = 0;
    private ByteBuffer configData = null;

    boolean inputFrameEnd = false;
    boolean encodePacketEnd = false;
    private final MediaCodec.BufferInfo codecBufferInfo = new MediaCodec.BufferInfo();

    private void checkOnMediaCodecThread() {
        if (mediaCodecThread.getId() != Thread.currentThread().getId()) {
            throw new RuntimeException("MediaCodecVideoEncoder previously operated on " + mediaCodecThread
                    + " but is now called on " + Thread.currentThread());
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


    public boolean initEncode(String  mime, int width, int height, int kbps, int fps, EglBase14.Context sharedContext){
        this.width =width;
        this.height = height;
        if(mediaCodecThread != null){
            throw new RuntimeException("Forgot to release()?");
        }
        runningInstance = this;

        targetBitrateBps = 1024*kbps;
        targetFps = fps;

        mediaCodecThread = Thread.currentThread();

        try{
            MediaFormat format = MediaFormat.createVideoFormat(mime, width, height);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            format.setInteger(MediaFormat.KEY_BIT_RATE, targetBitrateBps);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, targetFps);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, FRAME_INTERVAL);
            format.setInteger(MediaFormat.KEY_BITRATE_MODE, VIDEO_ControlRateConstant);

            if(mime.equals(MediaFormat.MIMETYPE_VIDEO_AVC)){
                format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);
                format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.AVCLevel3);
            }
            else if(mime.equals(MediaFormat.MIMETYPE_VIDEO_HEVC)){
                format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.HEVCProfileMain);
                format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.HEVCMainTierLevel31);
            }


            mediaCodec = createEncoderByType(mime);
            if (mediaCodec == null) {
                Logging.e(TAG, "Can not create media encoder");
                release();
                return false;
            }
            mediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

            eglBase = new EglBase14(sharedContext, EglBase.CONFIG_RECORDABLE);
            inputSurface = mediaCodec.createInputSurface();
            eglBase.createSurface(inputSurface);
            drawer = new GlRectDrawer();
            mediaCodec.start();
        }
        catch (IllegalStateException e){
            Logging.e(TAG, "initEncode failed", e);
            release();
            return false;
        }
        return true;

    }

    boolean encodeBuffer(int inputBuffer, int size, long presentationTimestampUs) {
        checkOnMediaCodecThread();
        try {
            mediaCodec.queueInputBuffer(inputBuffer, 0, size, presentationTimestampUs, 0);
            return true;
        } catch (IllegalStateException e) {
            Logging.e(TAG, "encodeBuffer failed", e);
            return false;
        }
    }

    boolean encodeTexture(int oesTextureId, float[] transformationMatrix, long presentationTimestampUs) {
        checkOnMediaCodecThread();
        try {
            eglBase.makeCurrent();
            // TODO(perkj): glClear() shouldn't be necessary since every pixel is covered anyway,
            // but it's a workaround for bug webrtc:5147.
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            drawer.drawOes(oesTextureId, transformationMatrix, width, height, 0, 0, width, height);
            eglBase.swapBuffers(TimeUnit.MICROSECONDS.toNanos(presentationTimestampUs));
            return true;
        } catch (RuntimeException e) {
            Logging.e(TAG, "encodeTexture failed", e);
            return false;
        }
    }


    public boolean sendFrame(int oesTextureId, float[] transformationMatrix, long presentationTimestampUs) {
        return encodeTexture(oesTextureId, transformationMatrix, presentationTimestampUs);
    }

    public void release() {
        Logging.d(TAG, "Java releaseEncoder");
        if(mediaCodecThread != null)
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
        if (drawer != null) {
            drawer.release();
            drawer = null;
        }
        if (eglBase != null) {
            eglBase.release();
            eglBase = null;
        }
        if (inputSurface != null) {
            inputSurface.release();
            inputSurface = null;
        }
        runningInstance = null;

        if (stopHung) {
            codecErrors++;
            if (errorCallback != null) {
                Logging.e(TAG, "Invoke codec error callback. Errors: " + codecErrors);
                errorCallback.onMediaCodecVideoEncoderCriticalError(codecErrors);
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

    int dequeueInputBuffer() {
        checkOnMediaCodecThread();
        try {
            return mediaCodec.dequeueInputBuffer(DEQUEUE_TIMEOUT);
        } catch (IllegalStateException e) {
            Logging.e(TAG, "dequeueIntputBuffer failed", e);
            return ERROR_UNKNOW;
        }
    }

    public void signalEndOfInputStream(){
        if(!inputFrameEnd){
            mediaCodec.signalEndOfInputStream();
            Logging.w(TAG, "signalEndOfInputStream");
            inputFrameEnd = true;
        }
    }
    public CodecBufferInfo flushEncoder(){
        checkOnMediaCodecThread();
        if(!encodePacketEnd){
//            Logging.w(TAG, "flush encode");
            return dequeueOutputBuffer(250 * 1000);
        }
        return null;
    }




    CodecBufferInfo dequeueOutputBuffer(int dequeueTimeoutUs){
//        int dequeueTimeoutUs = DEQUEUE_TIMEOUT;
        if(encodePacketEnd){
            return null;
        }
        checkOnMediaCodecThread();
        try {
            int result = mediaCodec.dequeueOutputBuffer(codecBufferInfo, dequeueTimeoutUs);
            // Check if this is config frame and save configuration data.
            if (result >= 0) {
                boolean isConfigFrame = (codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0;
                if (isConfigFrame) {
                    Logging.d(TAG, "Config frame generated. Offset: " + codecBufferInfo.offset + ". Size: " + codecBufferInfo.size);
                    configData = ByteBuffer.allocateDirect(codecBufferInfo.size);
                    ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(result);
                    outputBuffer.position(codecBufferInfo.offset);
                    outputBuffer.limit(codecBufferInfo.offset + codecBufferInfo.size);
                    configData.put(outputBuffer);
                    // Log few SPS header bytes to check profile and level.
                    String spsData = "";
                    for (int i = 0; i < (codecBufferInfo.size < 8 ? codecBufferInfo.size : 8); i++) {
                        spsData += Integer.toHexString(configData.get(i) & 0xff) + " ";
                    }
                    Logging.d(TAG, spsData);
                    // Release buffer back.
                    mediaCodec.releaseOutputBuffer(result, false);
                    // Query next output.
                    result = mediaCodec.dequeueOutputBuffer(codecBufferInfo, dequeueTimeoutUs);
                }
            }
            if (result >= 0) {
                if((codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0){
                    encodePacketEnd = true;
                    Logging.w(TAG, "encode end, no packet will be available after this");
                }

                // MediaCodec doesn't care about Buffer position/remaining/etc so we can
                // mess with them to get a slice and avoid having to pass extra
                // (BufferInfo-related) parameters back to C++.
                ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(result);
                outputBuffer.position(codecBufferInfo.offset);
                outputBuffer.limit(codecBufferInfo.offset + codecBufferInfo.size);
                // Check key frame flag.
                boolean isKeyFrame = (codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0;
                if (isKeyFrame) {
                    Logging.d(TAG, "Sync frame generated");
                }
                if (isKeyFrame) {
                    Logging.d(TAG, "Appending config frame of size " + configData.capacity()
                            + " to output buffer with offset " + codecBufferInfo.offset + ", size " + codecBufferInfo.size);
                    // For H.264 key frame append SPS and PPS NALs at the start
                    ByteBuffer keyFrameBuffer = ByteBuffer.allocateDirect(configData.capacity() + codecBufferInfo.size);
                    configData.rewind();
                    keyFrameBuffer.put(configData);
                    keyFrameBuffer.put(outputBuffer);
                    keyFrameBuffer.position(0);
                    releaseOutputBuffer(result);
                    return new CodecBufferInfo(result, keyFrameBuffer, keyFrameBuffer.limit(), 0, codecBufferInfo.flags,
                            codecBufferInfo.presentationTimeUs, isKeyFrame);
                } else {
                    ByteBuffer frameBuffer = ByteBuffer.allocateDirect(codecBufferInfo.size);
                    frameBuffer.put(outputBuffer);
                    frameBuffer.position(0);
                    releaseOutputBuffer(result);
                    return new CodecBufferInfo(result, frameBuffer, frameBuffer.limit(), 0, codecBufferInfo.flags,
                            codecBufferInfo.presentationTimeUs, isKeyFrame);
                }
            } else if (result == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
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
        } catch (IllegalStateException e) {
            Logging.e(TAG, "dequeueOutputBuffer failed", e);
            return new CodecBufferInfo(-1, null, 0, 0, 0,0,false);
        }
    }

    public CodecBufferInfo receivePacket(){
        return dequeueOutputBuffer(0);
    }


    public MediaFormat getMediaFormat(){
        return mediaFormat;
    }
    public ByteBuffer getConfigureData(){
        if(configData == null)
            return null;
        configData.rewind();
        return configData;
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
}















