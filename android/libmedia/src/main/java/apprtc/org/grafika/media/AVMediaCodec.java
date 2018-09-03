package apprtc.org.grafika.media;

import java.nio.ByteBuffer;

import static android.media.MediaCodec.BUFFER_FLAG_CODEC_CONFIG;

public interface AVMediaCodec {
    static final String TAG = "AVCodec";
    static int ERROR_TRY_AGAIN = -1;
    static int ERROR_EOF = -127;
    static int ERROR_UNKNOW= -128;
    final String  KEY_BIT_WIDTH = "bit-width";


    static int AV_CODEC_ID_H264 = 27;
    static int AV_CODEC_ID_AVC = AV_CODEC_ID_H264;
    static int AV_CODEC_ID_HEVC= 173;
    static int AV_CODEC_ID_VP8 = 139;
    static int AV_CODEC_ID_VP9= 167;


    static final int BUFFER_FLAG_EXTERNAL_DATA = BUFFER_FLAG_CODEC_CONFIG;

}
