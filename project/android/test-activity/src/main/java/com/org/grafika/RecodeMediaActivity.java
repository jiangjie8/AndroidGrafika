package com.org.grafika;

import android.os.Bundle;

import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;


import java.io.File;

import apprtc.org.grafika.AVRecodeInterface;

import apprtc.org.grafika.Logging;


public class RecodeMediaActivity extends AppCompatActivity {
    private static String TAG = "RecodeMediaActivity";

    private SurfaceView mSurfaceView;
    AVRecodeInterface mediaRecode;
    long start_time = 0;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_recode_media);
        mSurfaceView = findViewById(R.id.surfaceView);

    }

    int loopNumber = 0;

    void startRecoder() {
        mediaRecode = AVRecodeInterface.getRecodeInstance();
        mediaRecode.openInputSource("/sdcard/Download/2k.mp4");
        File dir = new File("/sdcard/Download/jie");
        if (!dir.exists()) {
            dir.mkdirs();
        }
        else{
            if (dir.isDirectory()) {
                String[] childrens = dir.list();
                for (String child : childrens) {
                    new File(dir, child).delete();
                }
            }
        }
        mediaRecode.setOutputSourceParm("/sdcard/Download/jie", "t_hevc_5m-%03d.mp4",
                960, 540, (int)(1.5 * 1024), 10000,
                540, -1);
//        mediaRecode.setOutputSourceParm("/sdcard/Download/jie", "hevc_1088p_2000kb-%03d.mp4",
//                1920, 1088, 2000, 100000);
        mediaRecode.setRecodeEventListener(new AVRecodeInterface.onRecodeEventListener() {
            @Override
            public void onPrintReport(final String message) {
                Logging.d(TAG, message);
            }

            @Override
            public void onErrorMessage(int errorCode, String errorMessage) {
                Logging.e(TAG, "error information: " + errorMessage);
            }

            @Override
            public void onOneFileGen(String fileName) {
                Logging.w(TAG, "one video file generation  " + fileName);
            }

            @Override
            public void onRecodeFinish() {
                if (mWork) {
                    Logging.e(TAG, "=== spend time  " + (System.currentTimeMillis() - start_time) / 1000.0);
                    start_time = System.currentTimeMillis();
//                    startRecoder();
                }
            }
        });

        mediaRecode.starRecode();
        start_time = System.currentTimeMillis();
        Logging.e(TAG, "=== startRecoder  loopNumber " + loopNumber++);
    }

    void stopRecoder() {
        mediaRecode.stopRecode();
    }

    private boolean mWork = false;

    public void start_onClick(@SuppressWarnings("unused") View unused) {
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.start_capture);
        mWork = !mWork;
        if (mWork) {
            startRecoder();
            Logging.w(TAG, "press button, start requested");
            fab.setImageResource(android.R.drawable.ic_media_pause);
        } else {
//            mHandler.sendMessage(mHandler.obtainMessage(MSE_STOP_RECODER));
            stopRecoder();
            Logging.w(TAG, "press button, Stop requested");
            fab.setImageResource(android.R.drawable.ic_media_play);
        }
    }


    @Override
    protected void onStart() {
        super.onStart();
        Logging.w(TAG, "onStart, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onResume() {
        super.onResume();
        Logging.w(TAG, "onResume, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onPause() {
        super.onPause();
        Logging.w(TAG, "onPause, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onStop() {
        super.onStop();
        Logging.w(TAG, "onStop, thread ID " + Thread.currentThread().getId());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        Logging.w(TAG, "onDestroy, thread ID " + Thread.currentThread().getId());
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
