package com.org.grafika;

import android.app.ListActivity;
import android.content.ContentValues;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.view.View;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import apprtc.org.grafika.Logging;


public class MainActivity extends ListActivity {
    public static final String TAG = "MainActivity";

    private static final String rootPackageName = "com.org.grafika.";

    // map keys
    private static final String TITLE = "title";
    private static final String DESCRIPTION = "description";
    private static final String CLASS_NAME = "class_name";

    /**
     * Each entry has three strings: the test title, the test description, and the name of
     * the activity class.
     */
    private static final String[][] TESTS = {
            { "TextureView and  SurfaceView",
                    "show with TextureView and  SurfaceView",
                    "ViewsActivity" },
            { "recode media",
                    "recode media",
                    "RecodeMediaActivity" },
    };

    /**
     * Compares two list items.
     */
    private static final Comparator<Map<String, Object>> TEST_LIST_COMPARATOR =
            new Comparator<Map<String, Object>>() {
                @Override
                public int compare(Map<String, Object> map1, Map<String, Object> map2) {
                    String title1 = (String) map1.get(TITLE);
                    String title2 = (String) map2.get(TITLE);
                    return title1.compareTo(title2);
                }
            };


    void getConfigure(){
        try{
            Uri uri = Uri.parse("content://com.hm.vmovie.contentprovider/config");
            Cursor cursor = getContentResolver().query(uri, null, null, null,
                    null);
            if(cursor != null && cursor.moveToNext()){
                Logging.e(TAG, "USB_ENCODER_RESOLUTION " + cursor.getInt(cursor.getColumnIndex("USB_ENCODER_RESOLUTION")));
                Logging.e(TAG, "USB_ENCODER_BITRATE " + cursor.getInt(cursor.getColumnIndex("USB_ENCODER_BITRATE")));
                Logging.e(TAG, "KEY_FRAME_INTERVAL " + cursor.getInt(cursor.getColumnIndex("KEY_FRAME_INTERVAL")));
                Logging.e(TAG, "AUDIO_BITRATE " + cursor.getInt(cursor.getColumnIndex("AUDIO_BITRATE")));
                Logging.e(TAG, "AUDIO_SAMPLING_RATE " + cursor.getInt(cursor.getColumnIndex("AUDIO_SAMPLING_RATE")));
                Logging.e(TAG, "RECORD_INTERVAL " + cursor.getInt(cursor.getColumnIndex("RECORD_INTERVAL")));
                Logging.e(TAG, "RECORD_ALARM " + cursor.getInt(cursor.getColumnIndex("RECORD_ALARM")));

            }
        }catch (Exception e){
            Logging.e(TAG, "error " + e.toString());
        }
    }

    void setConfigure(){
        try{
            Uri uri = Uri.parse("content://com.hm.vmovie.contentprovider/config");
            ContentValues values = new ContentValues();
            values.put("USB_ENCODER_RESOLUTION", 0);
            values.put("USB_ENCODER_BITRATE", 10000);
            values.put("KEY_FRAME_INTERVAL", 7);
            values.put("AUDIO_BITRATE", 2);
            values.put("AUDIO_SAMPLING_RATE", 0);
            values.put("RECORD_INTERVAL", 1);
            values.put("RECORD_ALARM", 1000);
            getContentResolver().update(uri, values, null, null);

        }catch (Exception e){
            Logging.e(TAG, "set error " + e.toString());
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        setListAdapter(new SimpleAdapter(this, createActivityList(),
                android.R.layout.two_line_list_item, new String[] { TITLE, DESCRIPTION },
                new int[] { android.R.id.text1, android.R.id.text2 } ));


//        setConfigure();
//        getConfigure();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!PermissionHelper.hasWriteStoragePermission(this)) {
            PermissionHelper.requestWriteStoragePermission(this);
        }
        if (!PermissionHelper.hasCameraPermission(this)) {
            PermissionHelper.requestCameraPermission(this, false);
        }
        Logging.w(TAG, "onResume, thread ID " + Thread.currentThread().getId());
    }

    /**
     * Creates the list of activities from the string arrays.
     */
    private List<Map<String, Object>> createActivityList() {
        List<Map<String, Object>> testList = new ArrayList<Map<String, Object>>();

        for (String[] test : TESTS) {
            Map<String, Object> tmp = new HashMap<String, Object>();
            tmp.put(TITLE, test[0]);
            tmp.put(DESCRIPTION, test[1]);
            Intent intent = new Intent();
            // Do the class name resolution here, so we crash up front rather than when the
            // activity list item is selected if the class name is wrong.
            try {
                Class cls = Class.forName(rootPackageName + test[2]);
                intent.setClass(this, cls);
                tmp.put(CLASS_NAME, intent);
            } catch (ClassNotFoundException cnfe) {
                throw new RuntimeException("Unable to find " + test[2], cnfe);
            }
            testList.add(tmp);
        }

        Collections.sort(testList, TEST_LIST_COMPARATOR);

        return testList;
    }

    @Override
    protected void onListItemClick(ListView listView, View view, int position, long id) {
        Map<String, Object> map = (Map<String, Object>)listView.getItemAtPosition(position);
        Intent intent = (Intent) map.get(CLASS_NAME);
        startActivity(intent);
    }

}