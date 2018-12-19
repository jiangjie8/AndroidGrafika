package apprtc.org.grafika.media;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayDeque;

import java.util.Queue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import apprtc.org.grafika.Logging;
import apprtc.org.grafika.Utils.ThreadUtils;
import apprtc.org.grafika.gles.GlRectDrawer;
import apprtc.org.grafika.media.AVStruct.CodecBufferInfo;


public class MediaCodecVideoDecoder implements AVMediaCodec{
    // This class is constructed, operated, and destroyed by its C++ incarnation,
    // so the class and its methods have non-public visibility.  The API this
    // class exposes aims to mimic the webrtc::VideoDecoder API as closely as
    // possibly to minimize the amount of translation work necessary.

    private static final String TAG = "MediaCodecVideoDecoder";

    // Tracks webrtc::VideoCodecType.
    public enum VideoCodecType {
        VIDEO_CODEC_VP8,
        VIDEO_CODEC_VP9,
        VIDEO_CODEC_H264;
    }

    // Timeout for input buffer dequeue.
    private static final int DEQUEUE_TIMEOUT = 0;
    // Timeout for codec releasing.
    private static final int MEDIA_CODEC_RELEASE_TIMEOUT_MS = 5000;
    // Max number of output buffers queued before starting to drop decoded frames.
    private static final int MAX_QUEUED_OUTPUTBUFFERS = 4;
    // Active running decoder instance. Set in initDecode() (called from native code)
    // and reset to null in release() call.
    private static MediaCodecVideoDecoder runningInstance = null;
    private static MediaCodecVideoDecoderErrorCallback errorCallback = null;
    private static int codecErrors = 0;
    // List of disabled codec types - can be set from application.

    private long m_input_packet = 0;
    private long m_output_frame = 0;

    private Thread mediaCodecThread;
    private MediaCodec mediaCodec;


    private int width;
    private int height;
    private boolean hasDecodedFirstFrame;
    private final boolean useSurface = true;

    // The below variables are only used when decoding to a Surface.
    private TextureListener textureListener;
    private int droppedFrames;
    private Surface surface = null;
    private final Queue<CodecBufferInfo> dequeuedSurfaceOutputBuffers = new ArrayDeque<CodecBufferInfo>();

    private boolean inputPacketEnd = false;
    private boolean decodeFrameEnd = false;
    private final MediaCodec.BufferInfo codecBufferInfo = new MediaCodec.BufferInfo();
    // MediaCodec error handler - invoked when critical error happens which may prevent
    // further use of media codec API. Now it means that one of media codec instances
    // is hanging and can no longer be used in the next call.
    public static interface MediaCodecVideoDecoderErrorCallback {
        void onMediaCodecVideoDecoderCriticalError(int codecErrors);
    }

    public static void setErrorCallback(MediaCodecVideoDecoderErrorCallback errorCallback) {
        Logging.d(TAG, "Set error callback");
        MediaCodecVideoDecoder.errorCallback = errorCallback;
    }



    public static void printStackTrace() {
        if (runningInstance != null && runningInstance.mediaCodecThread != null) {
            StackTraceElement[] mediaCodecStackTraces = runningInstance.mediaCodecThread.getStackTrace();
            if (mediaCodecStackTraces.length > 0) {
                Logging.d(TAG, "MediaCodecVideoDecoder stacks trace:");
                for (StackTraceElement stackTrace : mediaCodecStackTraces) {
                    Logging.d(TAG, stackTrace.toString());
                }
            }
        }
    }


    public MediaCodecVideoDecoder() {}

    private void checkOnMediaCodecThread() throws IllegalStateException {
        if (mediaCodecThread.getId() != Thread.currentThread().getId()) {
            throw new IllegalStateException("MediaCodecVideoDecoder previously operated on "
                    + mediaCodecThread + " but is now called on " + Thread.currentThread());
        }
    }

    // Pass null in |surfaceTextureHelper| to configure the codec for ByteBuffer output.
    public boolean initDecode(MediaFormat format, SurfaceTextureHelper surfaceTextureHelper) {
        if (mediaCodecThread != null) {
            throw new RuntimeException("initDecode: Forgot to release()?");
        }

        runningInstance = this; // Decoder is now running and can be queried for stack traces.
        mediaCodecThread = Thread.currentThread();
        try {
            this.width = format.getInteger(MediaFormat.KEY_WIDTH);
            this.height = format.getInteger(MediaFormat.KEY_HEIGHT);
            GlRectDrawer.set_Encoder_FULL_RECTANGLE(this.width, this.height);

//            MediaFormat myFormat = MediaFormat.createVideoFormat(format.getString(MediaFormat.KEY_MIME), format.getInteger(MediaFormat.KEY_WIDTH), format.getInteger(MediaFormat.KEY_HEIGHT));
//            myFormat.setByteBuffer("csd-0", format.getByteBuffer("csd-0"));
//            myFormat.setByteBuffer("csd-1", format.getByteBuffer("csd-1"));
//            myFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, format.getInteger(MediaFormat.KEY_MAX_INPUT_SIZE));

            if (useSurface) {
                textureListener = new TextureListener(surfaceTextureHelper);
                surface = new Surface(surfaceTextureHelper.getSurfaceTexture());
            }

            mediaCodec = MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME));
            if (mediaCodec == null) {
                Logging.e(TAG, "Can not create media decoder");
                return false;
            }
            mediaCodec.configure(format, surface, null, 0);
            mediaCodec.start();

            hasDecodedFirstFrame = false;
            dequeuedSurfaceOutputBuffers.clear();
            droppedFrames = 0;
            return true;
        } catch (IllegalStateException e) {
            Logging.e(TAG, "initDecode failed", e);
            return false;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    // Resets the decoder so it can start decoding frames with new resolution.
    // Flushes MediaCodec and clears decoder output buffers.
    private void reset(int width, int height) {
        if (mediaCodecThread == null || mediaCodec == null) {
            throw new RuntimeException("Incorrect reset call for non-initialized decoder.");
        }
        Logging.d(TAG, "Java reset: " + width + " x " + height);

        mediaCodec.flush();

        this.width = width;
        this.height = height;
//        decodeStartTimeMs.clear();
        dequeuedSurfaceOutputBuffers.clear();
        hasDecodedFirstFrame = false;
        droppedFrames = 0;
    }

    public void release() {
        Logging.d(TAG, "Java releaseDecoder. Total number of dropped frames: " + droppedFrames);
        if(mediaCodecThread != null)
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
                errorCallback.onMediaCodecVideoDecoderCriticalError(codecErrors);
            }
        }

        mediaCodec = null;
        mediaCodecThread = null;
        runningInstance = null;
        if (useSurface) {
            surface.release();
            surface = null;
            textureListener.release();
        }
        m_input_packet = 0;
        m_output_frame = 0;
        Logging.d(TAG, "Java releaseDecoder done");
    }

    // Dequeue an input buffer and return its index, -1 if no input buffer is
    // available, or -2 if the codec is no longer operative.
    int dequeueInputBuffer() {
        checkOnMediaCodecThread();
        try {
            int ret = mediaCodec.dequeueInputBuffer(DEQUEUE_TIMEOUT);
            return ret;
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
                m_input_packet++;
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



    // Helper struct for dequeueTextureBuffer() below.
    public static class DecodedTextureBuffer {
        public final int textureID;
        public final float[] transformMatrix;
        // Presentation timestamp returned in dequeueOutputBuffer call.
        public final long ptsUs;

        // A DecodedTextureBuffer with zero |textureID| has special meaning and represents a frame
        // that was dropped.
        public DecodedTextureBuffer(int textureID, float[] transformMatrix, long ptsUs) {
            this.textureID = textureID;
            this.transformMatrix = transformMatrix;
            this.ptsUs = ptsUs;

        }

    }

    // Poll based texture listener.
    private static class TextureListener
            implements SurfaceTextureHelper.OnTextureFrameAvailableListener {
        private final SurfaceTextureHelper surfaceTextureHelper;
        // |newFrameLock| is used to synchronize arrival of new frames with wait()/notifyAll().
        private final Object newFrameLock = new Object();
        // |bufferToRender| is non-null when waiting for transition between addBufferToRender() to
        // onTextureFrameAvailable().
        private CodecBufferInfo bufferToRender;
        private DecodedTextureBuffer renderedBuffer;

        public TextureListener(SurfaceTextureHelper surfaceTextureHelper) {
            this.surfaceTextureHelper = surfaceTextureHelper;
            surfaceTextureHelper.startListening(this);
        }

        public void addBufferToRender(CodecBufferInfo buffer) {
            if (bufferToRender != null) {
                Logging.e(TAG, "Unexpected addBufferToRender() called while waiting for a texture.");
                throw new IllegalStateException("Waiting for a texture.");
            }
            bufferToRender = buffer;
        }

        public boolean isWaitingForTexture() {
            synchronized (newFrameLock) {
                return bufferToRender != null;
            }
        }

        // Callback from |surfaceTextureHelper|. May be called on an arbitrary thread.
        @Override
        public void onTextureFrameAvailable(
                int oesTextureId, float[] transformMatrix, long timestampNs) {
            synchronized (newFrameLock) {
                if (renderedBuffer != null) {
                    Logging.e(
                            TAG, "Unexpected onTextureFrameAvailable() called while already holding a texture.");
                    throw new IllegalStateException("Already holding a texture.");
                }
                // |timestampNs| is always zero on some Android versions.
                renderedBuffer = new DecodedTextureBuffer(oesTextureId, transformMatrix, TimeUnit.NANOSECONDS.toMicros(timestampNs));
                bufferToRender = null;
                newFrameLock.notifyAll();
            }
        }

        // Dequeues and returns a DecodedTextureBuffer if available, or null otherwise.
        @SuppressWarnings("WaitNotInLoop")
        public DecodedTextureBuffer dequeueTextureBuffer(int timeoutMs) {
            synchronized (newFrameLock) {
                if (renderedBuffer == null && timeoutMs > 0 && isWaitingForTexture()) {
                    try {
                        newFrameLock.wait(timeoutMs);
                    } catch (InterruptedException e) {
                        // Restore the interrupted status by reinterrupting the thread.
                        Thread.currentThread().interrupt();
                    }
                }
                DecodedTextureBuffer returnedBuffer = renderedBuffer;
                renderedBuffer = null;
                return returnedBuffer;
            }
        }

        public void release() {
            // SurfaceTextureHelper.stopListening() will block until any onTextureFrameAvailable() in
            // progress is done. Therefore, the call must be outside any synchronized
            // statement that is also used in the onTextureFrameAvailable() above to avoid deadlocks.
            surfaceTextureHelper.stopListening();
            synchronized (newFrameLock) {
                if (renderedBuffer != null) {
                    surfaceTextureHelper.returnTextureFrame();
                    renderedBuffer = null;
                }
            }
        }

        public void returnTextureFrame(){
            surfaceTextureHelper.returnTextureFrame();
        }
    }

    // Returns null if no decoded buffer is available, and otherwise a DecodedByteBuffer.
    // Throws IllegalStateException if call is made on the wrong thread, if color format changes to an
    // unsupported format, or if |mediaCodec| is not in the Executing state. Throws CodecException
    // upon codec error.
    private CodecBufferInfo dequeueOutputBuffer(int timeoutUs) {
        checkOnMediaCodecThread();
        if(decodeFrameEnd){
            return null;
        }
        // Drain the decoder until receiving a decoded buffer or hitting
        // MediaCodec.INFO_TRY_AGAIN_LATER.
        while (true) {
            final int result = mediaCodec.dequeueOutputBuffer(codecBufferInfo, timeoutUs);
            if(result == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED){
            }
            else if(result == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                MediaFormat format = mediaCodec.getOutputFormat();
//                Logging.w(TAG, "INFO_OUTPUT_FORMAT_CHANGED");
            }
            else if(result == MediaCodec.INFO_TRY_AGAIN_LATER){
                return null;
            }
            else if(result < 0){
                throw new RuntimeException("Unexpected dequeueOutputBuffer error");
            }
            else {
                if((codecBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0){
                    decodeFrameEnd = true;
                    Logging.i(TAG, "decode end, no frame will be available after this");
                }
                return new CodecBufferInfo(result, codecBufferInfo.size, codecBufferInfo.offset, codecBufferInfo.flags, codecBufferInfo.presentationTimeUs, false);
            }
        }
    }

    // Returns null if no decoded buffer is available, and otherwise a DecodedTextureBuffer.
    // Throws IllegalStateException if call is made on the wrong thread, if color format changes to an
    // unsupported format, or if |mediaCodec| is not in the Executing state. Throws CodecException
    // upon codec error. If |dequeueTimeoutMs| > 0, the oldest decoded frame will be dropped if
    // a frame can't be returned.
     DecodedTextureBuffer dequeueTextureBuffer(int dequeueTimeoutUs) {
//        int dequeueTimeoutUs = DEQUEUE_TIMEOUT;
        checkOnMediaCodecThread();
        if (!useSurface) {
            throw new IllegalStateException("dequeueTexture() called for byte buffer decoding.");
        }


        if (dequeuedSurfaceOutputBuffers.size() < MAX_QUEUED_OUTPUTBUFFERS /2){
            CodecBufferInfo outputBuffer = dequeueOutputBuffer(dequeueTimeoutUs);
            if (outputBuffer != null) {
                dequeuedSurfaceOutputBuffers.add(outputBuffer);
            }
        }

        MaybeRenderDecodedTextureBuffer();
        // Check if there is texture ready now by waiting max |dequeueTimeoutMs|.
        DecodedTextureBuffer renderedBuffer = textureListener.dequeueTextureBuffer(dequeueTimeoutUs);
        if (renderedBuffer != null) {
            MaybeRenderDecodedTextureBuffer();
            m_output_frame++;
            return renderedBuffer;
        }

        if (dequeuedSurfaceOutputBuffers.size()
                >= Math.min(MAX_QUEUED_OUTPUTBUFFERS,  mediaCodec.getOutputBuffers().length)) {
            ++droppedFrames;
            // Drop the oldest frame still in dequeuedSurfaceOutputBuffers.
            // The oldest frame is owned by |textureListener| and can't be dropped since
            // mediaCodec.releaseOutputBuffer has already been called.
            final CodecBufferInfo droppedFrame = dequeuedSurfaceOutputBuffers.remove();
            if (dequeueTimeoutUs > 0) {
                // TODO(perkj): Re-add the below log when VideoRenderGUI has been removed or fixed to
                // return the one and only texture even if it does not render.
                Logging.e(TAG, "Draining decoder. Dropping frame with TS: "
                        + droppedFrame.ptsUs + ". Total number of dropped frames: "
                        + droppedFrames);
            } else {
                Logging.e(TAG, "Too many output buffers " + dequeuedSurfaceOutputBuffers.size()
                        + ". Dropping frame with TS: " + droppedFrame.ptsUs
                        + ". Total number of dropped frames: " + droppedFrames);
            }
            mediaCodec.releaseOutputBuffer(droppedFrame.index, false /* render */);
//            return new DecodedTextureBuffer(0, null, droppedFrame.presentationTimeStampMs);
        }
        return null;
    }

    public DecodedTextureBuffer receiveFrame(){
        return dequeueTextureBuffer(0);
    }

    public DecodedTextureBuffer flushDecoder(){
        checkOnMediaCodecThread();
        if(!decodeFrameEnd){
            DecodedTextureBuffer buffer = dequeueTextureBuffer(200 * 1000);
            if(buffer == null){
//                if(dequeuedSurfaceOutputBuffers.size() == 0)
                if(!inputPacketEnd){
                    sendPacket(null, 0, 0);
                }
                buffer = dequeueTextureBuffer(500 * 1000);
            }
            if(buffer != null)
                return buffer;
        }
        if(m_input_packet != m_output_frame)
            Logging.e(TAG, "inputPacket number " + m_input_packet + ",  outputFrame number " + m_output_frame);
        return null;
    }

    private void MaybeRenderDecodedTextureBuffer() {
        if (dequeuedSurfaceOutputBuffers.isEmpty() || textureListener.isWaitingForTexture()) {
            return;
        }
        // Get the first frame in the queue and render to the decoder output surface.
        final CodecBufferInfo buffer = dequeuedSurfaceOutputBuffers.remove();

        textureListener.addBufferToRender(buffer);
        mediaCodec.releaseOutputBuffer(buffer.index, true /* render */);
    }


    void returnTextureFrame(){
        textureListener.returnTextureFrame();
    }

    // Release a dequeued output byte buffer back to the codec for re-use. Should only be called for
    // non-surface decoding.
    // Throws IllegalStateException if the call is made on the wrong thread, if codec is configured
    // for surface decoding, or if |mediaCodec| is not in the Executing state. Throws
    // MediaCodec.CodecException upon codec error.
//    private void returnDecodedOutputBuffer(int index)
//            throws IllegalStateException, MediaCodec.CodecException {
//        checkOnMediaCodecThread();
//        if (useSurface) {
//            throw new IllegalStateException("returnDecodedOutputBuffer() called for surface decoding.");
//        }
//        mediaCodec.releaseOutputBuffer(index, false /* render */);
//    }

}
