package apprtc.org.grafika.media;


import static android.media.MediaCodec.BUFFER_FLAG_CODEC_CONFIG;

public interface AVMediaCodec {
    static final String TAG = "AVCodec";
    static int ERROR_TRY_AGAIN = -1;
    static int ERROR_EOF = -127;
    static int ERROR_UNKNOW= -128;
    final String  KEY_BIT_WIDTH = "bit-width";


    static int AV_CODEC_ID_H264 = 27;               // AV_CODEC_ID_H264 use ffmpeg value;
    static int AV_CODEC_ID_AVC = AV_CODEC_ID_H264;
    static int AV_CODEC_ID_HEVC= 173;
    static int AV_CODEC_ID_VP8 = 139;
    static int AV_CODEC_ID_VP9= 167;

    static int AV_PIX_FMT_YUV420P = 0; // AV_PIX_FMT_YUV420P use ffmpeg value;
    static int AV_PIX_FMT_NV12 = 23;


    static final int BUFFER_FLAG_EXTERNAL_DATA = BUFFER_FLAG_CODEC_CONFIG;

}
