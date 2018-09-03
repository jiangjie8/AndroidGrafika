package apprtc.org.grafika.media;

import android.media.MediaFormat;

import java.nio.ByteBuffer;

public class AVStruct {



    public final static class AVStream{
        public final int tarckIndex;
        public final int mediaType;
        public final MediaFormat format;
        public Object mediaCodec;

        AVStream(MediaFormat format, int mediaType, int tarckIndex){
            this.format = format;
            this.mediaType = mediaType;
            this.tarckIndex = tarckIndex;
        }

        void setMediaCodec(Object mediaCodec){
            this.mediaCodec = mediaCodec;
        }
    }


    public final static class MediaInfo{
        public int bitrate = 0;
        public long duration = 0;
        public long startTime = 0;

        public int videoCodecID;
        public int width = 0;
        public int height = 0;
        public int framerate = 0;

        public int audioCodecID;
        public int channels = 0;
        public int sampleRate = 0;
        public int sampleDepth = 0;
        public int frameSize = 0;
        MediaInfo(){
        }
    }


    public final static class AVPacket{
        //public int trackIndex;
        public int mediaType;
        public long ptsUs;
        public long dtsUs;
        public ByteBuffer buffer;
        public int bufferSize;
        public int bufferOffset;
        public int bufferFlags;
        public AVPacket(int mediaType, long ptsUs, long dtsUs, ByteBuffer buffer, int bufferSize, int bufferOffset, int bufferFlags){
            this.mediaType = mediaType;
            this.ptsUs = ptsUs;
            this.dtsUs = dtsUs;
            this.buffer = buffer;
            this.bufferSize = bufferSize;
            this.bufferOffset = bufferOffset;
            this.bufferFlags = bufferFlags;
        }

        public AVPacket(int size){
            this.mediaType = AVMediaType.UNKNOWN;
            this.ptsUs = 0;
            this.dtsUs = 0;
            this.buffer = ByteBuffer.allocateDirect(size);
            this.bufferSize = 0;
            this.bufferOffset = 0;
            this.bufferFlags = 0;
        }
        public AVPacket(){
            this.mediaType = AVMediaType.UNKNOWN;
            this.ptsUs = 0;
            this.dtsUs = 0;
            this.buffer = null;
            this.bufferSize = 0;
            this.bufferOffset = 0;
            this.bufferFlags = 0;
        }

    }

    public final static class AVMediaType {
        final public static int UNKNOWN = -1;
        final public static int VIDEO = 0;
        final public static int AUDIO = 1;
        final public static int DATA = 2;
        final public static int SUBTITLE = 3;
        final public static int ATTACHMENT = 4;
        final public static int NB = 5;

//        final int type;
//
//        AVMediaType(int type){
//            this.type =type;
//        }

    };



    // Helper struct for dequeueOutputBuffer() below.
    public final static class CodecBufferInfo {
        public CodecBufferInfo(
                int index, int size, int offset, int bufferFlags, long ptsUs, boolean isKeyFrame) {
            this.index = index;
            this.buffer = null;
            this.flags = bufferFlags;
            this.size = size;
            this.offset = offset;
            this.isKeyFrame = isKeyFrame;
            this.ptsUs = ptsUs;
        }
        public CodecBufferInfo(
                int index, ByteBuffer buffer, int size, int offset, int bufferFlags, long ptsUs, boolean isKeyFrame) {
            this.index = index;
            this.buffer = buffer;
            this.flags = bufferFlags;
            this.size = size;
            this.offset = offset;
            this.isKeyFrame = isKeyFrame;
            this.ptsUs = ptsUs;
        }

        public CodecBufferInfo(
                int index, ByteBuffer buffer, int size, long ptsUs) {
            this.index = index;
            this.buffer = buffer;
            this.size = size;
            this.offset = 0;
            this.ptsUs = ptsUs;
            this.flags = 0;
            this.isKeyFrame = false;
        }

        public CodecBufferInfo(ByteBuffer buffer, CodecBufferInfo ob) {
            this.index = ob.index;
            this.buffer = buffer;
            this.flags = ob.flags;
            this.size = ob.size;
            this.offset = ob.offset;
            this.isKeyFrame = ob.isKeyFrame;
            this.ptsUs = ob.ptsUs;
        }
        public final int index;
        public final ByteBuffer buffer;
        public final int size;
        public final int offset;
        public final int flags;
        public final boolean isKeyFrame;
        public final long ptsUs;
    }


}
