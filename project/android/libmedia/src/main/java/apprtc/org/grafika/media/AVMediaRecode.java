package apprtc.org.grafika.media;

import android.graphics.Bitmap;
import android.media.MediaFormat;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import apprtc.org.grafika.AVRecodeInterface;
import apprtc.org.grafika.JNIBridge;
import apprtc.org.grafika.Logging;
import apprtc.org.grafika.gles.EglBase;
import apprtc.org.grafika.gles.EglBase14;
import apprtc.org.grafika.gles.GlRectDrawer;
import apprtc.org.grafika.gles.GlUtil;
import apprtc.org.grafika.media.AVStruct.*;

import static apprtc.org.grafika.media.AVMediaCodec.AV_CODEC_ID_AVC;
import static apprtc.org.grafika.media.AVMediaCodec.AV_CODEC_ID_HEVC;
import static apprtc.org.grafika.media.AVMediaCodec.AV_CODEC_ID_VP8;
import static apprtc.org.grafika.media.AVMediaCodec.AV_CODEC_ID_VP9;
import static apprtc.org.grafika.media.AVMediaCodec.BUFFER_FLAG_EXTERNAL_DATA;
import static apprtc.org.grafika.media.AVStruct.AVMediaType.VIDEO;

public class AVMediaRecode implements AVRecodeInterface {

    private static String TAG = "AVMediaRecode";
    private final int EncoderID = AV_CODEC_ID_AVC;

    private MediaCodecVideoDecoder mVideoDecoder = null;
    private MediaCodecVideoEncoder mVideoEncoder = null;

    private Handler mHandler = null;
    private EglBase rootEglBase = null;
    private SurfaceTextureHelper mSurfaceTextureHelper = null;

    private MediaInfo mMediaInfo = new MediaInfo();
    private AVPacket mAVPacketBuffer = new AVPacket((int) (1920 * 1080 * 1.5));

    private ByteBuffer mExternalBuffer = null;

    private long mEngineHandleDemuxer = 0;

    private boolean recodeIsRunning = false;
    private int mErrorCode = 0;

    private String inputSource;
    private String clipDirectory;
    private String clipPrefix;
    private int clipBitrate = 0;
    private int clipWidth = 0;
    private int clipHeight = 0;
    private int clipIndex = 0;
    private int thumbnailWidth = 0;
    private int thumbnailHeight = 0;
    private long clipDurationUs;


    private static final ExecutorService executor = Executors.newSingleThreadExecutor();
    private onRecodeEventListener eventListener = null;
    private onRecodeEventListener eventListener_inner = new onRecodeEventListener() {
        @Override
        public void onReportMessage(final MediaInfo mediaInfo, final int code, final String message){
            mErrorCode = code;
            if(code != ERROR_NONE){
                Logging.e(TAG, message);
            }
            if(eventListener != null){
                executor.execute(new Runnable() {
                    @Override
                    public void run() {
                        eventListener.onReportMessage(mediaInfo, code, message);
                    }
                });
            }
        }

        @Override
        public void onOneFileGen(String fileName) {
            if(eventListener != null){
                executor.execute(new Runnable() {
                    @Override
                    public void run() {
                        eventListener.onOneFileGen(fileName);
                    }
                });
            }
        }

        @Override
        public void onRecodeFinish() {
            if(eventListener != null){
                executor.execute(new Runnable() {
                    @Override
                    public void run() {
                        eventListener.onRecodeFinish();
                    }
                });
            }
        }
    };

    class StreamInfo{
        long startUs = Long.MAX_VALUE;
        long durationUs = Long.MAX_VALUE;
        int index = -1;
        String output = null;
        String thumbnail = null;

        StreamInfo(long startUs, long durationUs, int index){
            this.startUs = startUs;
            this.durationUs = durationUs;
            this.index = index;
        }
    }

    private StreamInfo streamInfo = null;

    public AVMediaRecode(){ }

    private boolean initVideoDecoder(){
        MediaFormat format = null;
        if(mMediaInfo.width <= 0 || mMediaInfo.height <= 0){
            return false;
        }

        if(mMediaInfo.videoCodecID == AV_CODEC_ID_AVC){
            format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, mMediaInfo.width, mMediaInfo.height);
        }
        else if(mMediaInfo.videoCodecID == AV_CODEC_ID_HEVC){
            format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_HEVC, mMediaInfo.width, mMediaInfo.height);
        }
        else if(mMediaInfo.videoCodecID == AV_CODEC_ID_VP8){
            format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_VP8, mMediaInfo.width, mMediaInfo.height);
        }
        else if(mMediaInfo.videoCodecID == AV_CODEC_ID_VP9){
            format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_VP9, mMediaInfo.width, mMediaInfo.height);
        }
        else{
            return false;
        }
        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,(int)(2.5*1024*1024));
        format.setInteger(MediaFormat.KEY_FRAME_RATE, mMediaInfo.framerate);
        mVideoDecoder = new MediaCodecVideoDecoder();
        return mVideoDecoder.initDecode(format, mSurfaceTextureHelper);
    }

    private boolean initVideoEncoder(int codecID, int width, int height, int bitrate, int fps, EglBase.Context sharedContext){
        if(mVideoEncoder != null)
            throw new RuntimeException("forget  release encoder?");

        if(sharedContext instanceof EglBase14.Context){
            String  mime = MediaFormat.MIMETYPE_VIDEO_AVC;
            mVideoEncoder = new MediaCodecVideoEncoder();
            if(codecID == AV_CODEC_ID_HEVC){
                mime = MediaFormat.MIMETYPE_VIDEO_HEVC;
            }
            return mVideoEncoder.initEncode(mime, width, height, bitrate, fps, (EglBase14.Context) sharedContext);
        }
        else{
            eventListener_inner.onReportMessage(mMediaInfo, ERROR_EGL, "unsupport EGL14");
            return false;
        }
    }


    private AVPacket readPacket(){
        if(mAVPacketBuffer.bufferSize == 0){
            while(true){
                mAVPacketBuffer.buffer.rewind();
                int ret = JNIBridge.native_demuxer_readPacket(mEngineHandleDemuxer, mAVPacketBuffer);
                if(ret < 0)
                    return null;
                if(mAVPacketBuffer.bufferSize == 0)
                    continue;
                if(mAVPacketBuffer.mediaType == VIDEO){
                    break;
                }
            }
            mAVPacketBuffer.buffer.position(0);
            mAVPacketBuffer.buffer.limit(mAVPacketBuffer.bufferSize);
        }
        return new AVPacket(mAVPacketBuffer.mediaType, mAVPacketBuffer.ptsUs, mAVPacketBuffer.dtsUs, mAVPacketBuffer.buffer,
                mAVPacketBuffer.bufferSize, mAVPacketBuffer.bufferOffset, mAVPacketBuffer.bufferFlags);
    }

    boolean advance(){
        mAVPacketBuffer.bufferSize = 0;
        mAVPacketBuffer.buffer.rewind();
        return true;
    }

    boolean writePacket(AVPacket packet){
        int ret = -1;
        if(packet.bufferSize == 0 && packet.ptsUs == 0 && packet.dtsUs == 0){
            return  true;
        }
        if(mEngineHandleDemuxer != 0){
            getExternalData();
            if(mExternalBuffer == null){
                eventListener_inner.onReportMessage(mMediaInfo, ERROR_BUG, "no sps pps data before write video packet");
            }
            ret = JNIBridge.native_demuxer_writePacket(mEngineHandleDemuxer, packet);
            if(ret < 0){
                eventListener_inner.onReportMessage(mMediaInfo, ret, "write packet error, error code: " + ret);
            }
        }
        return ret == 0;
    }

    void getExternalData(){
        if(mExternalBuffer == null){
            ByteBuffer buffer = mVideoEncoder.getConfigureData();
            if(buffer != null){
                mExternalBuffer = ByteBuffer.allocateDirect(buffer.limit());
                mExternalBuffer.put(buffer);
                mExternalBuffer.rewind();
                buffer.rewind();
                JNIBridge.native_demuxer_writePacket(mEngineHandleDemuxer, new AVPacket(VIDEO, Long.MAX_VALUE, Long.MAX_VALUE,
                        mExternalBuffer, mExternalBuffer.limit(), 0, BUFFER_FLAG_EXTERNAL_DATA));
            }
        }
    }


    void recodeLoopThread(){
        if(mVideoDecoder == null || mEngineHandleDemuxer == 0){
            release();
            eventListener_inner.onRecodeFinish();
            mHandler.getLooper().quitSafely();
            return;
        }

        while(recodeIsRunning && mErrorCode == 0){
            AVPacket packet = readPacket();
            if(packet == null) {
                break;
            }
            int ret = sendPacket(packet.buffer, packet.bufferSize, packet.ptsUs, packet.mediaType);
            if(ret == 0){
                advance();
            }

            if(mVideoDecoder != null){
                MediaCodecVideoDecoder.DecodedTextureBuffer textureBuffer = mVideoDecoder.receiveFrame();
                if(textureBuffer != null){
                    if(generatorClip(textureBuffer.ptsUs) < 0)
                        break;
                    encoderWrietPacket(textureBuffer.textureID, textureBuffer.transformMatrix, textureBuffer.ptsUs);
                    mSurfaceTextureHelper.returnTextureFrame();
                }
            }

        }

        //flush video decoder
        while (mVideoDecoder != null && mErrorCode == 0){
            MediaCodecVideoDecoder.DecodedTextureBuffer textureBuffer = mVideoDecoder.flushDecoder();
            if(textureBuffer == null)
                break;
            generatorClip(textureBuffer.ptsUs);
            encoderWrietPacket(textureBuffer.textureID, textureBuffer.transformMatrix, textureBuffer.ptsUs);
            mSurfaceTextureHelper.returnTextureFrame();

        }

        flushEncoder();
        release();
        eventListener_inner.onRecodeFinish();
        mHandler.getLooper().quitSafely();
    }




    int generatorClip(long ptsUs){
        int ret = 0;

        if(streamInfo != null && (ptsUs >= streamInfo.startUs + streamInfo.durationUs)){
            clipIndex++;
            flushEncoder();
            streamInfo = null;
        }

        if(streamInfo == null){
            long duration = clipDurationUs;

            if(mMediaInfo.duration  - (clipIndex + 1) * clipDurationUs < TimeUnit.SECONDS.toMicros(3)){
                duration = mMediaInfo.duration - clipIndex* clipDurationUs + TimeUnit.SECONDS.toMicros(1);
            }
            ptsUs = clipDurationUs * (ptsUs / clipDurationUs);

            streamInfo = new StreamInfo(ptsUs, duration, clipIndex);
            streamInfo.output = String.format(clipDirectory + clipPrefix, clipIndex);
            Logging.i(TAG, "new output " + streamInfo.output);
            MediaInfo mediaInfo = new MediaInfo();
            mediaInfo.videoCodecID = EncoderID;
            mediaInfo.width = clipWidth;
            mediaInfo.height = clipHeight;
            ret = JNIBridge.native_demuxer_openOutputFormat(mEngineHandleDemuxer, streamInfo.output, mediaInfo);
            if(ret < 0){
                eventListener_inner.onReportMessage(mMediaInfo, ret,
                        String.format("open output file %s error, error code: %d", streamInfo.output, ret));
                return  ret;
            }
            if(mVideoDecoder != null){
                if(!initVideoEncoder(EncoderID, clipWidth, clipHeight, clipBitrate,
                        mMediaInfo.framerate, rootEglBase.getEglBaseContext())){
                    if(mVideoEncoder != null){mVideoEncoder.release(); mVideoEncoder = null;}
                    eventListener_inner.onReportMessage(mMediaInfo, ERROR_ENCODE,  "init video encode error");
                }
            };
        }
        return ret;
    }

    boolean encoderWrietPacket(int oesTextureId, float[] transformationMatrix, long presentationTimestampUs){
        if(mVideoEncoder == null)
            return false;

        boolean ret = mVideoEncoder.sendFrame(oesTextureId, transformationMatrix, presentationTimestampUs);

        if(ret ){

            if(streamInfo.thumbnail == null){

                streamInfo.thumbnail = streamInfo.output.substring(0, streamInfo.output.lastIndexOf(".")) + ".jpg";;

                if(thumbnailHeight == -1 && mMediaInfo.height > 0 && mMediaInfo.width > 0){
                    thumbnailHeight =  thumbnailWidth * mMediaInfo.height / mMediaInfo.width;
                }
                else{
                    thumbnailHeight = thumbnailWidth * clipHeight / clipWidth;
                }

                newThumbnail( streamInfo.thumbnail, thumbnailWidth, thumbnailHeight, oesTextureId, transformationMatrix);
            }
        }

        while (ret) {
            AVPacket pkt = null;
            CodecBufferInfo bufferinfo = mVideoEncoder.receivePacket();
            if(bufferinfo != null) {
                pkt =  new AVPacket(VIDEO, bufferinfo.ptsUs, bufferinfo.ptsUs, bufferinfo.buffer,
                        bufferinfo.size, bufferinfo.offset, bufferinfo.flags);
            }
            if (pkt == null)
                break;
            if(!(ret = writePacket(pkt)))
                break;
        }
        return ret;
    }


    void flushEncoder(){
        //flush video encoder
        while(mVideoEncoder != null){
            AVPacket  packet = null;
            CodecBufferInfo bufferinfo = mVideoEncoder.flushEncoder();
            if(bufferinfo != null) {
                packet = new AVPacket(VIDEO, bufferinfo.ptsUs, bufferinfo.ptsUs, bufferinfo.buffer,
                        bufferinfo.size, bufferinfo.offset, bufferinfo.flags);
            }
            if(packet == null){
                break;
            }

            writePacket(packet);
            mVideoEncoder.signalEndOfInputStream();
        }
        int ret = JNIBridge.native_demuxer_closeOutputFormat(mEngineHandleDemuxer);
        if(ret < 0){
            eventListener_inner.onReportMessage(mMediaInfo, ret,
                    String.format("close output file %s error, error code %d", streamInfo.output, ret));
        }else{
            Logging.i(TAG,"one video file generation  " + streamInfo.output);
            eventListener_inner.onOneFileGen(streamInfo.output);
        }
        if(mVideoEncoder != null){mVideoEncoder.release(); mVideoEncoder = null;}
    }

    int sendPacket(ByteBuffer buffer, int size, long pts, int mediaType){
        int ret = -1;
        if(mediaType == VIDEO){
            ret = mVideoDecoder.sendPacket(buffer, size, pts);
        }
        return ret;
    }
    void release()
    {
        if(mEngineHandleDemuxer != 0){JNIBridge.native_demuxer_closeInputFormat(mEngineHandleDemuxer); mEngineHandleDemuxer = 0;}
        if(mVideoDecoder != null){ mVideoDecoder.release(); mVideoDecoder = null; }
        if(mVideoEncoder != null){ mVideoEncoder.release(); mVideoEncoder = null; }
        if(mSurfaceTextureHelper != null){mSurfaceTextureHelper.dispose();}
        if(rootEglBase != null){rootEglBase.release();}
        Logging.i(TAG, "!!!!!!!!release all source");
    }

    @Override
    public boolean openInputSource(String input) {
        this.inputSource = input;
        if(mHandler == null){
            final HandlerThread thread = new HandlerThread("vp_recode");
            thread.start();
            mHandler = new Handler(thread.getLooper(), mCallback);
        }
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mEngineHandleDemuxer = JNIBridge.native_demuxer_createEngine();
                int ret = JNIBridge.native_demuxer_openInputFormat(mEngineHandleDemuxer, inputSource);
                if(ret < 0){
                    eventListener_inner.onReportMessage(mMediaInfo, ret,
                            String.format("open input file %s error, error code %d", inputSource, ret));
                    return;
                }
                JNIBridge.native_demuxer_getMediaInfo(mEngineHandleDemuxer, mMediaInfo);

                Logging.i(TAG, "source " + inputSource + ",duration " + mMediaInfo.duration + ",startTime " + mMediaInfo.startTime +
                        ",videoCodecID(h264=27) " + mMediaInfo.videoCodecID +
                        ",width " + mMediaInfo.width + ",height " + mMediaInfo.height + ",framerate " + mMediaInfo.framerate +
                        ",pixfmt(yuv420p=0) " + mMediaInfo.pixfmt);
                Logging.i(TAG, "audioCodecID " + mMediaInfo.audioCodecID + ",channels " + mMediaInfo.channels + ",sampleRate " + mMediaInfo.sampleRate +
                        ",sampleDepth " + mMediaInfo.sampleDepth + ",frameSize " + mMediaInfo.frameSize);

//                if(mMediaInfo.pixfmt != AV_PIX_FMT_YUV420P){
//                    eventListener_inner.onErrorMessage(-1, "only support yuv420p, this format is " + mMediaInfo.pixfmt);
//                    Logging.e(TAG, "only support yuv420p, this format is " + mMediaInfo.pixfmt);
//                    return;
//                }

                rootEglBase = EglBase.create();
                mSurfaceTextureHelper = SurfaceTextureHelper.create("vp_texturehelp", rootEglBase.getEglBaseContext());
                if(mSurfaceTextureHelper == null){
                    eventListener_inner.onReportMessage(mMediaInfo, ERROR_EGL, "create SurfaceTexture error");
                    return;
                }

                if(!initVideoDecoder()){
                    if(mVideoDecoder != null){mVideoDecoder.release(); mVideoDecoder = null;}
                    eventListener_inner.onReportMessage(mMediaInfo, ERROR_DECODE, "init video decoder error");
                }

            }
        });

        return true;
    }


    private void newThumbnail(String name, int width, int height, int oesTextureId, float[] transformationMatrix){

        if(new File(name).exists()){
            Logging.i(TAG, "thumbnail exist.");
            return;
        }

        int quality = 80;
        GlRectDrawer glRectDrawer = null;
        BufferedOutputStream bos = null;
        try {
            ByteBuffer buf = ByteBuffer.allocateDirect(width * height * 4);
            buf.order(ByteOrder.LITTLE_ENDIAN);

            glRectDrawer = new GlRectDrawer();
            glRectDrawer.drawOes(oesTextureId, transformationMatrix, width, height, 0, 0, width, height);
            GLES20.glReadPixels(0, 0, width, height,
                    GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buf);
            GlUtil.checkNoGLES2Error("glReadPixels");

            buf.rewind();
            ByteBuffer flip_buffer = ByteBuffer.allocateDirect(width * height * 4);
            buf.order(ByteOrder.LITTLE_ENDIAN);
            for(int i = height - 1; i >= 0; i--){
                buf.get(flip_buffer.array(), i * width * 4,width*4);
            }
            flip_buffer.rewind();


            Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            bos = new BufferedOutputStream(new FileOutputStream(name));
            bmp.copyPixelsFromBuffer(flip_buffer);
            bmp.compress(Bitmap.CompressFormat.JPEG, quality, bos);
            bmp.recycle();
        }catch (Exception e){
            Logging.e(TAG, "new thumbnail error", e);
        }
        finally {
            if(bos != null){
                try {
                    bos.close();
                } catch (IOException e) {
                    Logging.e(TAG, "BufferedOutputStream close error", e);
                }
            }
        }

        if(glRectDrawer != null){
            glRectDrawer.release();
        }

    }


    @Override
    public boolean setOutputSourceParm(String clipDirectory, String clipPrefix, int clipWidth, int clipHeight, int clipBitrate, int clipDurationMs,
                                       int thumbnailWidth, int thumbnailHeight) {
        this.clipPrefix = clipPrefix;
        this.clipDurationUs = TimeUnit.MILLISECONDS.toMicros(clipDurationMs);
        this.clipWidth = clipWidth;
        this.clipHeight = clipHeight;
        this.thumbnailWidth = thumbnailWidth;
        this.thumbnailHeight = thumbnailHeight;
        this.clipBitrate = clipBitrate;
        this.clipDirectory = clipDirectory;
        if(clipDirectory.charAt(clipDirectory.length() -1) != File.separatorChar){
            this.clipDirectory = this.clipDirectory + File.separatorChar;
        }
        return true;
    }

    @Override
    public void setRecodeEventListener(onRecodeEventListener listener){
        eventListener = listener;
    }

    @Override
    public void starRecode() {
        mHandler.sendMessage(mHandler.obtainMessage(MSE_START_RECODER, this));

    }

    @Override
    public void stopRecode() {
        recodeIsRunning = false;
    }

    @Override
    public void waitRecode() {
        try {
            mHandler.getLooper().getThread().join();
        } catch (InterruptedException e) {
            Logging.e(TAG, "wait error " + e.toString());
        }
    }

    static private final int MSE_START_RECODER = 0x0001;
    static private final int MSE_STOP_RECODER = 0x0002;
    static private Handler.Callback mCallback = new Handler.Callback() {
        @Override
        public boolean handleMessage(Message msg) {
            final AVMediaRecode thisObj = (AVMediaRecode) msg.obj;
            switch (msg.what) {
                case MSE_START_RECODER: {
                    if (!thisObj.recodeIsRunning) {
                        thisObj.recodeIsRunning = true;
                        thisObj.recodeLoopThread();
                    }
                    break;
                }
                case MSE_STOP_RECODER: {
//                stopRecoder();
                    break;
                }
                default:
                    throw new RuntimeException("Unknown message " + msg.what);
            }
            return true;
        }
    };


//    static public DumpH264 fileDump = new DumpH264("/sdcard/Download/jie/jie.h264");
//    static class DumpH264{
//        private File file;
//        private FileOutputStream fos;
//
//        public DumpH264(String name){
//            file  = new File(name);
//            if(file.exists()){
//                file.delete();
//            }
//
//            try {
//                fos = new FileOutputStream(file);
//            } catch (FileNotFoundException e) {
//                e.printStackTrace();
//            }
//        }
//
//        public void Write(ByteBuffer bb, int size){
//
//            try {
//                ByteBuffer aa = ByteBuffer.allocate(bb.limit());
//                aa.put(bb);
//                aa.rewind();
//                bb.rewind();
//                fos.write(aa.array(),0, size);
//                fos.flush();
//                //Log.w(TAG, "arrayOffset " + aa.arrayOffset()+ " position " + aa.position()+ " limit " + aa.limit());
//            } catch (IOException e) {
//                e.printStackTrace();
//            }
//        }
//    }

}
