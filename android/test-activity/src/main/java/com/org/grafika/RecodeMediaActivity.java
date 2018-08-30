package com.org.grafika;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

import android.view.SurfaceView;
import android.view.View;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.Map;
import java.util.Queue;

import apprtc.org.grafika.gles.EglBase;
import apprtc.org.grafika.gles.EglBase14;

import apprtc.org.grafika.media.AVMediaRecode;
import apprtc.org.grafika.media.MediaCodecAudioDecoder;
import apprtc.org.grafika.media.MediaCodecAudioEncoder;
import apprtc.org.grafika.media.MediaCodecVideoDecoder;
import apprtc.org.grafika.media.MediaCodecVideoDecoder.DecodedTextureBuffer;
import apprtc.org.grafika.media.MediaCodecVideoEncoder;
import apprtc.org.grafika.media.SurfaceTextureHelper;


import static apprtc.org.grafika.media.AVMediaCodec.ERROR_TRY_AGAIN;
import static apprtc.org.grafika.media.AVMediaCodec.KEY_BIT_WIDTH;


public class RecodeMediaActivity extends AppCompatActivity {
    private static String TAG = "RecodeMediaActivity";

    private SurfaceView mSurfaceView;
    AVMediaRecode mediaRecode;



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_recode_media);
        mSurfaceView = findViewById(R.id.surfaceView);



    }

    int loopNumber = 0;
    void startRecoder(){
        mediaRecode = new AVMediaRecode();
        mediaRecode.initParm("/sdcard/Download/small.mp4", "/sdcard/Download/jie", "jie-",
                960, 540, 1500, 5000);

        mediaRecode.setRecodeEventListener(new AVMediaRecode.RecodeEventListener() {
            @Override
            public void onRecodeFinish() {
                if(mWork){
                    startRecoder();
                }
            }
        }, new Handler(getMainLooper()));

        mediaRecode.startRecoder();
        Log.e(TAG, "=== startRecoder  loopNumber " + loopNumber++);
    }

    void stopRecoder(){
        mediaRecode.stopRecoder();
    }

    private boolean mWork = false;
    public void start_onClick(@SuppressWarnings("unused") View unused) {
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.start_capture);
        mWork = !mWork;
        if(mWork){
            startRecoder();
            Log.w(TAG, "press button, start requested");
            fab.setImageResource (android.R.drawable.ic_media_pause);
        }
        else{
//            mHandler.sendMessage(mHandler.obtainMessage(MSE_STOP_RECODER));
            stopRecoder();
            Log.w(TAG, "press button, Stop requested");
            fab.setImageResource (android.R.drawable.ic_media_play);
        }
    }



    @Override
    protected void onStart() {
        super.onStart();
        Log.w(TAG, "onStart, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.w(TAG, "onResume, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.w(TAG, "onPause, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.w(TAG, "onStop, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        Log.w(TAG, "onDestroy, thread ID " + Thread.currentThread().getId());
    }


//    int selectTrack(MediaExtractor extractor, String mediaType){
//        int numTracks = extractor.getTrackCount();
//        for(int i = 0; i < numTracks; i++){
//            MediaFormat format = extractor.getTrackFormat(i);
//            String mime = format.getString(MediaFormat.KEY_MIME);
//            if(mime.startsWith(mediaType)){
//                Log.d(TAG, "Extractor selected track " + i + " (" + mime + "): " + format);
//                return i;
//            }
//
//        }
//        return -1;
//    }


//    DumpH264 fileDump = new DumpH264("/sdcard/Download/jie/jie.h264");
//    class DumpH264{
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
