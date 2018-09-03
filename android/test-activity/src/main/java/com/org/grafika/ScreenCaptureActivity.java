package com.org.grafika;

import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.Toast;

import apprtc.org.grafika.JNIBridge;
import apprtc.org.grafika.gles.EglBase;
import apprtc.org.grafika.gles.EglBase14;
import apprtc.org.grafika.gles.GlRectDrawer;
import apprtc.org.grafika.gles.GlUtil;

public class ScreenCaptureActivity extends AppCompatActivity implements SurfaceTexture.OnFrameAvailableListener {
    private static String TAG = "PreviewActivity";
    private static final int REQUEST_CODE_CAPTURE_PERM = 0x0001;

    private TextureView mTextureView;
    private MediaProjectionManager mediaProjectionManager;
    private MediaProjection mediaProjection;

    private GlRectDrawer mRectDraw;
    private final float[] mSTMatrix = new float[16];

    private int mTextureID;
    private EglBase14 mEglBase_view;
    private SurfaceTexture mSurfaceTexture = null;

    private int mWidth = 720;
    private int mHeight = 1280;

    private MediaCodecThread mEncodeThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getSupportActionBar().hide();
        setContentView(R.layout.activity_screencapture);

        mEncodeThread = new MediaCodecThread();
        mEncodeThread.startThread();
        mEglBase_view = new EglBase14(null, EglBase.CONFIG_RECORDABLE);
        mEglBase_view.createDummyPbufferSurface();

        mediaProjectionManager = (MediaProjectionManager)getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        mTextureView = (TextureView)findViewById(R.id.textureView);
        mTextureView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                if(mEglBase_view.hasSurface()){
                    mEglBase_view.releaseSurface();
                    mEglBase_view.createSurface(surface);
                }
                mEglBase_view.makeCurrent();
                mRectDraw = new GlRectDrawer();

                Log.w(TAG, "onSurfaceTextureAvailable, thread ID " + Thread.currentThread().getId() + ", width " + width + ", height " + height);
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
                Log.w(TAG, "onSurfaceTextureSizeChanged, thread ID " + Thread.currentThread().getId() + ", width " + width + ", height " + height);
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                if(mEglBase_view != null && mEglBase_view.hasSurface()){
                    mEglBase_view.releaseSurface();
                    mEglBase_view.createDummyPbufferSurface();
                }
                mRectDraw.release();
                mRectDraw = null;
                Log.w(TAG, "onSurfaceTextureDestroyed, thread ID " + Thread.currentThread().getId());
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {
//                Log.e(TAG, "onSurfaceTextureUpdated, thread ID " + Thread.currentThread().getId());
            }
        });
        Log.w(TAG, "onCreate, thread ID " + Thread.currentThread().getId());
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        if(mSurfaceTexture == null) return;
        mEglBase_view.makeCurrent();
        mSurfaceTexture.updateTexImage();
        mSurfaceTexture.getTransformMatrix(mSTMatrix);
        final long timestamp = mSurfaceTexture.getTimestamp();

        mEncodeThread.frameAvailable(timestamp, mSTMatrix);

        if(mRectDraw != null){
            mEglBase_view.makeCurrent();
            GLES20.glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            mRectDraw.drawOes(mTextureID, mSTMatrix, mWidth, mHeight, 0, 0, mWidth, mHeight);
            drawExtra(mWidth, mHeight);
            mEglBase_view.swapBuffers();
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
        if (!PermissionHelper.hasWriteStoragePermission(this)) {
            PermissionHelper.requestWriteStoragePermission(this);
        }
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
        releaseCaptureEncode();
        if(mEglBase_view != null){
            mEglBase_view.release();
            mEglBase_view = null;
        }
        mEncodeThread.threadQuit();
        Log.w(TAG, "onDestroy, thread ID " + Thread.currentThread().getId());
    }


    public void start_onClick(@SuppressWarnings("unused") View unused) {
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.start_capture);

        if(!mEncodeThread.mIsEncode){
            fab.setImageResource (android.R.drawable.ic_media_pause);
            Intent permissionIntent = mediaProjectionManager.createScreenCaptureIntent();
            startActivityForResult(permissionIntent, REQUEST_CODE_CAPTURE_PERM);
            logAndToast("createCodec  startCodec");
        }
        else{
            fab.setImageResource (android.R.drawable.ic_media_play);
            releaseCaptureEncode();
            logAndToast( "stopCodec  deleteCodec");
        }
    }
    private void startCapture(){
        DisplayManager dm = (DisplayManager)getSystemService(Context.DISPLAY_SERVICE);
        Display defaultDisplay;
        if(dm != null){
            defaultDisplay = dm.getDisplay(Display.DEFAULT_DISPLAY);
        }
        else {
            throw new IllegalStateException("Cannot display manager?!?");
        }

        if(defaultDisplay == null){
            throw new RuntimeException("No display found.");
        }
        DisplayMetrics metrics = getResources().getDisplayMetrics();
        int screenDensity = metrics.densityDpi;
        if(metrics.widthPixels > metrics.heightPixels){
            mWidth = 1280;
            mHeight = 720;
        }


        mTextureID = GlUtil.generateTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
        mSurfaceTexture = new SurfaceTexture(mTextureID);
        mSurfaceTexture.setOnFrameAvailableListener(ScreenCaptureActivity.this);
        mSurfaceTexture.setDefaultBufferSize(mWidth, mHeight);
        mediaProjection.createVirtualDisplay("jie play", mWidth, mHeight, 400,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR|DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC | DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION,
                new Surface(mSurfaceTexture), null, null);


    }

    private void releaseCaptureEncode() {
        mEncodeThread.stopEncode();
        if (mediaProjection != null) {
            mediaProjection.stop();
        }
        if(mSurfaceTexture != null){
            mSurfaceTexture.setOnFrameAvailableListener(null);
            mSurfaceTexture.release();
            mSurfaceTexture = null;
        }
    }


    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (REQUEST_CODE_CAPTURE_PERM == requestCode) {

            if (resultCode == RESULT_OK) {
                mediaProjection = mediaProjectionManager.getMediaProjection(resultCode, intent);
                startCapture();
                mEncodeThread.startEncode();
            } else {
                // user did not grant permissions
                new AlertDialog.Builder(this) .setTitle("Error") .setMessage("Permission is required to record the screen.")
                        .setNeutralButton(android.R.string.ok, null).show();
            }
        }
    }







    private class MediaCodecThread {
        private  HandlerThread  mHandlerThread;
        private Looper mLooper;

        private Surface mCodecInputSurface = null;

        private EglBase14 mEglBase;
        private GlRectDrawer mRectDraw;
        private final float[] mSTMatrix = new float[16];
        public boolean mIsEncode = false;

        public void startThread(){
            mHandlerThread = new HandlerThread("CodecThread");
            mHandlerThread.start();
            mLooper = mHandlerThread.getLooper();
        }

        public void startEncode(){
            new Handler(mLooper).post(new Runnable() {
                @Override
                public void run() {
                    JNIBridge.createCodec_MediaCodec();
                    mCodecInputSurface = JNIBridge.configureCodec_MediaCodec(mWidth, mHeight, true);
                    JNIBridge.startCodec_MediaCodec();

                    mEglBase = new EglBase14(mEglBase_view.getEglBaseContext(), EglBase.CONFIG_RECORDABLE);
                    mEglBase.createSurface(mCodecInputSurface);
                    mEglBase.makeCurrent();
                    mRectDraw = new GlRectDrawer();
                    mIsEncode = true;
                }
            });
        }

        public void frameAvailable(final long timestamp, final float[] matrix){
            new Handler(mLooper).post(new Runnable() {
                @Override
                public void run() {
                    if(!mIsEncode) return;
                    mEglBase.makeCurrent();
                    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
                    mRectDraw.drawOes(mTextureID, matrix, mWidth, mHeight, 0, 0, mWidth, mHeight);
                    mEglBase.swapBuffers(timestamp);
                }
            });
        }

        public void stopEncode(){
            new Handler(mLooper).post(new Runnable() {
                @Override
                public void run() {
                    if(!mIsEncode) return;
                    mIsEncode = false;
                    JNIBridge.stopCodec_MediaCodec();
                    JNIBridge.deleteCodec_MediaCodec();

                    mCodecInputSurface.release();
                    mCodecInputSurface = null;
                    mRectDraw.release();
                    mRectDraw = null;
                    mEglBase.release();
                    mEglBase = null;

                }
            });
        }

        public void threadQuit(){
            new Handler(mLooper).post(new Runnable() {
                @Override
                public void run() {
                    mLooper.quit();
                    mLooper = null;
                }
            });
        }
    }


    static private Toast logToast;
    private void logAndToast(final String msg) {
        new Handler(getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "logAndToast " + msg);
                if (logToast != null) {
                    logToast.cancel();
                }
                logToast = Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT);
                logToast.show();
            }
        });
    }

    static int mFrameNum = 0;
    private static void drawExtra(int width, int height) {

        int val = mFrameNum % 3;
        switch (val) {
            case 0:  GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);   break;
            case 1:  GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);   break;
            case 2:  GLES20.glClearColor(0.0f, 0.0f, 1.0f, 1.0f);   break;
        }

        int xpos = (int) (width * ((mFrameNum % 100) / 100.0f));
        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
        GLES20.glScissor(xpos, 0, width / 10, height / 2);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        mFrameNum++;
    }
}
