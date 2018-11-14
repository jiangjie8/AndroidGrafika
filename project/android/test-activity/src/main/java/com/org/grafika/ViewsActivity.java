package com.org.grafika;

import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.Display;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.WindowManager;

import java.io.IOException;

import apprtc.org.grafika.Logging;
import apprtc.org.grafika.gles.EglBase;
import apprtc.org.grafika.gles.EglBase14;
import apprtc.org.grafika.gles.GlRectDrawer;
import apprtc.org.grafika.gles.GlUtil;

public class ViewsActivity extends AppCompatActivity implements SurfaceTexture.OnFrameAvailableListener{
    private static String TAG = "ViewsActivity";

    private Camera mCamera;
    private int mCameraPreviewWidth;
    private int mCameraPreviewHeight;
    private int mTextureId;
    private SurfaceTexture mSurfaceTexture;
    private EglBase14 mEglBase_shared;
    private Object mLock = new Object();

    private TextureView mTextureView;
    private HandlerThread mRenderThread_TextureView;
    private EglBase14 mEgl_TextureView;
    private GlRectDrawer mRectDraw_TextureView;


    private SurfaceView mSurfaceView;
    private HandlerThread mRenderThread_SurfaceView;
    private EglBase14 mEgl_SurfaceView;
    private GlRectDrawer mRectDraw_SurfaceView;



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
//        getSupportActionBar().hide();
        setContentView(R.layout.activity_views);
        mTextureView = findViewById(R.id.textureView);
        mSurfaceView = findViewById(R.id.surfaceView);


        createSharedContext();

        mTextureView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(final SurfaceTexture surface, int width, int height) {

                mRenderThread_TextureView = new HandlerThread("TextureView_Render");
                mRenderThread_TextureView.start();

                new Handler(mRenderThread_TextureView.getLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        mEgl_TextureView = new EglBase14(mEglBase_shared.getEglBaseContext(), EglBase.CONFIG_RECORDABLE);
                        mEgl_TextureView.createSurface(surface);
                        mEgl_TextureView.makeCurrent();
                        mRectDraw_TextureView = new GlRectDrawer();
                    }
                });
                Logging.w(TAG, "onSurfaceTextureAvailable ");
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
                Logging.w(TAG, "onSurfaceTextureSizeChanged   size=" + width + "x" + height);
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                new Handler(mRenderThread_TextureView.getLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (mLock){
                            mRectDraw_TextureView.release();
                            mRectDraw_TextureView = null;
                            mEgl_TextureView.release();
                            mEgl_TextureView = null;
                            mRenderThread_TextureView.quit();
                            mRenderThread_TextureView = null;
                        }
                    }
                });
                Logging.w(TAG, "onSurfaceTextureDestroyed, thread ID " + Thread.currentThread().getId());
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {
//                Log.w(TAG, "onSurfaceTextureUpdated, thread ID " + Thread.currentThread().getId());
            }
        });


        mSurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                mRenderThread_SurfaceView = new HandlerThread("SurfaceView_Render");
                mRenderThread_SurfaceView.start();
                new Handler(mRenderThread_SurfaceView.getLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        mEgl_SurfaceView = new EglBase14(mEglBase_shared.getEglBaseContext(), EglBase.CONFIG_RECORDABLE);
                        mEgl_SurfaceView.createSurface(holder.getSurface());
                        mEgl_SurfaceView.makeCurrent();
                        mRectDraw_SurfaceView = new GlRectDrawer();
                    }
                });
                Logging.w(TAG, "surfaceCreated ");
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

                Logging.w(TAG, "surfaceChanged fmt=" + format + " size=" + width + "x" + height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                new Handler(mRenderThread_SurfaceView.getLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (mLock){
                            mRectDraw_SurfaceView.release();
                            mRectDraw_SurfaceView = null;
                            mEgl_SurfaceView.release();
                            mEgl_SurfaceView = null;
                            mRenderThread_SurfaceView.quit();
                            mRenderThread_SurfaceView = null;
                        }
                    }
                });
                Logging.w(TAG, "surfaceDestroyed, thread ID " + Thread.currentThread().getId());
            }
        });
        Logging.w(TAG, "onCreate, thread ID " + Thread.currentThread().getId());
    }
    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture)
    {

//        Log.w(TAG, "onFrameAvailable, thread ID " + Thread.currentThread().getId());
        if(mEglBase_shared == null)
            return;
        mEglBase_shared.makeCurrent();
        surfaceTexture.updateTexImage();
        final float[] STMatrix = new float[16];
        surfaceTexture.getTransformMatrix(STMatrix);
        final  long timestamp = surfaceTexture.getTimestamp();

        synchronized (mLock){
            if(mRenderThread_TextureView != null)
                new Handler(mRenderThread_TextureView.getLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        if(mEgl_TextureView == null) return;
                        mEgl_TextureView.makeCurrent();
                        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
                        mRectDraw_TextureView.drawOes(mTextureId, STMatrix, mCameraPreviewWidth, mCameraPreviewHeight,
                                0, 0, mCameraPreviewWidth, mCameraPreviewHeight);
                        mEgl_TextureView.swapBuffers(timestamp);

                    }
                });

        }

        synchronized (mLock){
            if(mRenderThread_SurfaceView != null)
                new Handler(mRenderThread_SurfaceView.getLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        if(mEgl_SurfaceView == null) return;
                        mEgl_SurfaceView.makeCurrent();
                        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
                        mRectDraw_SurfaceView.drawOes(mTextureId, STMatrix, mCameraPreviewWidth, mCameraPreviewHeight,
                                0, 0, mCameraPreviewWidth, mCameraPreviewHeight);
                        mEgl_SurfaceView.swapBuffers(timestamp);
                    }
                });
        }

    }
    private void startPreview(SurfaceTexture st){
        try{
            mCamera.setPreviewTexture(st);
        } catch (IOException e) {
            e.printStackTrace();
        }
        mCamera.startPreview();

    }
    private void createSharedContext(){
        mEglBase_shared = new EglBase14(null, EglBase.CONFIG_PLAIN);
        mEglBase_shared.createDummyPbufferSurface();
        mEglBase_shared.makeCurrent();
        mTextureId = GlUtil.generateTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
        mSurfaceTexture = new SurfaceTexture(mTextureId);
        mSurfaceTexture.setOnFrameAvailableListener(this);
    }

    private void startCameraCapture(){
        openCamera(1280, 720);
        startPreview(mSurfaceTexture);
    }

    private void stopCameraCapture() {
        releaseCamera();
        mSurfaceTexture.setOnFrameAvailableListener(null);
        mSurfaceTexture.release();
        mSurfaceTexture = null;
        mEglBase_shared.release();
        mEglBase_shared = null;

    }

    private void openCamera(int desiredWidth, int desiredHeight){
        if(mCamera != null){
            throw new RuntimeException("camera already initialized");
        }
        Camera.CameraInfo info = new Camera.CameraInfo();
        int  numCameras = android.hardware.Camera.getNumberOfCameras();
        for(int i = 0; i < numCameras; i++){
            Camera.getCameraInfo(i, info);
            if(info.facing == Camera.CameraInfo.CAMERA_FACING_BACK){
                mCamera = Camera.open(i);
                break;
            }
        }
        if(mCamera == null){
            Logging.e(TAG, "No front-facing camera found; opening default");
            mCamera = Camera.open();
        }
        if(mCamera == null){
            throw new RuntimeException("camera open failed");
        }
        Camera.Parameters parms = mCamera.getParameters();
        CameraUtils.choosePreviewSize(parms, desiredWidth, desiredHeight);
        parms.setRecordingHint(true);
        mCamera.setParameters(parms);

        int[] fpsRange = new int[2];
        Camera.Size mCameraPreviewSize = parms.getPreviewSize();
        parms.getPreviewFpsRange(fpsRange);
        String previewFacts = mCameraPreviewSize.width + "x" + mCameraPreviewSize.height;
        if(fpsRange[0] == fpsRange[1]){
            previewFacts += " @" + (fpsRange[0] / 1000.0) + "fps";

        }
        else{
            previewFacts += " @[" + (fpsRange[0] / 1000.0) + " - " + (fpsRange[1] / 1000.0) + "] fps";
        }
        Logging.w(TAG, previewFacts);

        mCameraPreviewWidth = mCameraPreviewSize.width;
        mCameraPreviewHeight = mCameraPreviewSize.height;

        Display display = ((WindowManager)getSystemService(WINDOW_SERVICE)).getDefaultDisplay();

        if(display.getRotation() == Surface.ROTATION_0) {
            mCamera.setDisplayOrientation(90);
        } else if(display.getRotation() == Surface.ROTATION_270) {
            mCamera.setDisplayOrientation(180);
        } else {
            // Set the preview aspect ratio.
        }

    }

    private void releaseCamera(){
        if(mCamera != null){
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
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
        startCameraCapture();
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
        stopCameraCapture();
        Logging.w(TAG, "onDestroy, thread ID " + Thread.currentThread().getId());
    }



}
