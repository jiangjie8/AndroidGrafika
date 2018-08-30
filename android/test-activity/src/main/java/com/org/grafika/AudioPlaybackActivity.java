package com.org.grafika;

import android.app.Activity;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.view.MotionEventCompat;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.SimpleAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

import com.org.grafika.audio_device.AudioDeviceListEntry;
import com.org.grafika.audio_device.AudioDeviceSpinner;

import apprtc.org.grafika.JNIBridge;


public class AudioPlaybackActivity extends Activity {

    private static final String TAG = MainActivity.class.getName();
    private static final long UPDATE_LATENCY_EVERY_MILLIS = 1000;
    private static final Integer[] CHANNEL_COUNT_OPTIONS = {1, 2, 3, 4, 5, 6, 7, 8};
    // Default to Stereo (OPTIONS is zero-based array so index 1 = 2 channels)
    private static final int CHANNEL_COUNT_DEFAULT_OPTION_INDEX = 1;
    private static final int[] BUFFER_SIZE_OPTIONS = {0, 1, 2, 4, 8};
    private static final String[] AUDIO_API_OPTIONS = {"Unspecified", "OpenSL ES", "AAudio"};

    private Spinner mAudioApiSpinner;
    private AudioDeviceSpinner mPlaybackDeviceSpinner;
    private Spinner mChannelCountSpinner;
    private Spinner mBufferSizeSpinner;
    private TextView mLatencyText;
    private Timer mLatencyUpdater;

    /*
     * Hook to user control to start / stop audio playback:
     *    touch-down: start, and keeps on playing
     *    touch-up: stop.
     * simply pass the events to native side.
     */
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = MotionEventCompat.getActionMasked(event);
        switch (action) {
            case (MotionEvent.ACTION_DOWN):
                setToneOn_AudioOboe(true);
                break;
            case (MotionEvent.ACTION_UP):
                setToneOn_AudioOboe(false);
                break;
        }
        return super.onTouchEvent(event);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_playback);

        setupAudioApiSpinner();
        setupPlaybackDeviceSpinner();
        setupChannelCountSpinner();
        setupBufferSizeSpinner();

        // initialize native audio system
        create_AudioOboe();

        // Periodically update the UI with the output stream latency
        mLatencyText = findViewById(R.id.latencyText);
        setupLatencyUpdater();
    }

    private void setupChannelCountSpinner() {
        mChannelCountSpinner = findViewById(R.id.channelCountSpinner);

        ArrayAdapter<Integer> channelCountAdapter = new ArrayAdapter<Integer>(this, R.layout.channel_counts_spinner, CHANNEL_COUNT_OPTIONS);
        mChannelCountSpinner.setAdapter(channelCountAdapter);
        mChannelCountSpinner.setSelection(CHANNEL_COUNT_DEFAULT_OPTION_INDEX);

        mChannelCountSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                setChannelCount_AudioOboe(CHANNEL_COUNT_OPTIONS[mChannelCountSpinner.getSelectedItemPosition()]);
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });
    }

    private void setupBufferSizeSpinner() {
        mBufferSizeSpinner = findViewById(R.id.bufferSizeSpinner);
        mBufferSizeSpinner.setAdapter(new SimpleAdapter(
                this,
                createBufferSizeOptionsList(), // list of buffer size options
                R.layout.buffer_sizes_spinner, // the xml layout
                new String[]{getString(R.string.buffer_size_description_key)}, // field to display
                new int[]{R.id.bufferSizeOption} // View to show field in
        ));

        mBufferSizeSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                setBufferSizeInBursts_AudioOboe(getBufferSizeInBursts());
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });
    }

    private void setupPlaybackDeviceSpinner() {
        mPlaybackDeviceSpinner = findViewById(R.id.playbackDevicesSpinner);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mPlaybackDeviceSpinner.setDirectionType(AudioManager.GET_DEVICES_OUTPUTS);
            mPlaybackDeviceSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                    setAudioDeviceId_AudioOboe(getPlaybackDeviceId());
                }

                @Override
                public void onNothingSelected(AdapterView<?> adapterView) {

                }
            });
        }
    }

    private void setupAudioApiSpinner() {
        mAudioApiSpinner = findViewById(R.id.audioApiSpinner);
        mAudioApiSpinner.setAdapter(new SimpleAdapter(
                this,
                createAudioApisOptionsList(),
                R.layout.audio_apis_spinner, // the xml layout
                new String[]{getString(R.string.audio_api_description_key)}, // field to display
                new int[]{R.id.audioApiOption} // View to show field in
        ));

        mAudioApiSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                setAudioApi_AudioOboe(i);
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });
    }

    private int getPlaybackDeviceId() {
        return ((AudioDeviceListEntry) mPlaybackDeviceSpinner.getSelectedItem()).getId();
    }

    private int getBufferSizeInBursts() {
        @SuppressWarnings("unchecked")
        HashMap<String, String> selectedOption = (HashMap<String, String>)
                mBufferSizeSpinner.getSelectedItem();

        String valueKey = getString(R.string.buffer_size_value_key);

        // parseInt will throw a NumberFormatException if the string doesn't contain a valid integer
        // representation. We don't need to worry about this because the values are derived from
        // the BUFFER_SIZE_OPTIONS int array.
        return Integer.parseInt(selectedOption.get(valueKey));
    }

    private void setupLatencyUpdater() {

        //Update the latency every 1s
        TimerTask latencyUpdateTask = new TimerTask() {
            @Override
            public void run() {

                final String latencyStr;

                if (isLatencyDetectionSupported_AudioOboe()) {

                    double latency = getCurrentOutputLatencyMillis_AudioOboe();
                    if (latency >= 0) {
                        latencyStr = String.format(Locale.getDefault(), "%.2fms", latency);
                    } else {
                        latencyStr = "Unknown";
                    }
                } else {
                    latencyStr = getString(R.string.only_supported_on_api_26);
                }

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mLatencyText.setText(getString(R.string.latency, latencyStr));
                    }
                });
            }
        };

        mLatencyUpdater = new Timer();
        mLatencyUpdater.schedule(latencyUpdateTask, 0, UPDATE_LATENCY_EVERY_MILLIS);

    }

    @Override
    protected void onDestroy() {
        if (mLatencyUpdater != null) mLatencyUpdater.cancel();
        delete_AudioOboe();
        super.onDestroy();
    }

    /**
     * Creates a list of buffer size options which can be used to populate a SimpleAdapter.
     * Each option has a description and a value. The description is always equal to the value,
     * except when the value is zero as this indicates that the buffer size should be set
     * automatically by the audio engine
     *
     * @return list of buffer size options
     */
    private List<HashMap<String, String>> createBufferSizeOptionsList() {

        ArrayList<HashMap<String, String>> bufferSizeOptions = new ArrayList<>();

        for (int i : BUFFER_SIZE_OPTIONS) {
            HashMap<String, String> option = new HashMap<>();
            String strValue = String.valueOf(i);
            String description = (i == 0) ? getString(R.string.automatic) : strValue;
            option.put(getString(R.string.buffer_size_description_key), description);
            option.put(getString(R.string.buffer_size_value_key), strValue);

            bufferSizeOptions.add(option);
        }

        return bufferSizeOptions;
    }

    private List<HashMap<String, String>> createAudioApisOptionsList() {

        ArrayList<HashMap<String, String>> audioApiOptions = new ArrayList<>();

        for (int i = 0; i < AUDIO_API_OPTIONS.length; i++) {
            HashMap<String, String> option = new HashMap<>();
            option.put(getString(R.string.buffer_size_description_key), AUDIO_API_OPTIONS[i]);
            option.put(getString(R.string.buffer_size_value_key), String.valueOf(i));
            audioApiOptions.add(option);
        }
        return audioApiOptions;
    }



    //lib oboe
    static long mEngineHandleAudioOboe = 0;
    static boolean create_AudioOboe(){

        if (mEngineHandleAudioOboe == 0){
            mEngineHandleAudioOboe = JNIBridge.native_createEngine_audioOboe();
        }
        return (mEngineHandleAudioOboe != 0);
    }

    static void delete_AudioOboe(){
        if (mEngineHandleAudioOboe != 0){
            JNIBridge.native_deleteEngine_audioOboe(mEngineHandleAudioOboe);
        }
        mEngineHandleAudioOboe = 0;
    }

    static void setToneOn_AudioOboe(boolean isToneOn){
        if (mEngineHandleAudioOboe != 0) JNIBridge.native_setToneOn_audioOboe(mEngineHandleAudioOboe, isToneOn);
    }

    static void setAudioApi_AudioOboe(int audioApi){
        if (mEngineHandleAudioOboe != 0) JNIBridge.native_setAudioApi_audioOboe(mEngineHandleAudioOboe, audioApi);
    }

    static void setAudioDeviceId_AudioOboe(int deviceId){
        if (mEngineHandleAudioOboe != 0) JNIBridge.native_setAudioDeviceId_audioOboe(mEngineHandleAudioOboe, deviceId);
    }

    static void setChannelCount_AudioOboe(int channelCount) {
        if (mEngineHandleAudioOboe != 0) JNIBridge.native_setChannelCount_audioOboe(mEngineHandleAudioOboe, channelCount);
    }

    static void setBufferSizeInBursts_AudioOboe(int bufferSizeInBursts){
        if (mEngineHandleAudioOboe != 0) JNIBridge.native_setBufferSizeInBursts_audioOboe(mEngineHandleAudioOboe, bufferSizeInBursts);
    }

    static double getCurrentOutputLatencyMillis_AudioOboe(){
        if (mEngineHandleAudioOboe == 0) return 0;
        return JNIBridge.native_getCurrentOutputLatencyMillis_audioOboe(mEngineHandleAudioOboe);
    }

    static boolean isLatencyDetectionSupported_AudioOboe() {
        return mEngineHandleAudioOboe != 0 && JNIBridge.native_isLatencyDetectionSupported_audioOboe(mEngineHandleAudioOboe);
    }



}
