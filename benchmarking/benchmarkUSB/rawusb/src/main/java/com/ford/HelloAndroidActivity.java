package com.ford;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;

import android.app.Activity;
import android.app.ListActivity;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;

import android.widget.ArrayAdapter;

public class HelloAndroidActivity extends ListActivity {

    private static String TAG = "rawusb";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		Log.i(TAG, "onCreate");

        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        Map<String, UsbDevice> devices = manager.getDeviceList();

        List<String> deviceNames = new ArrayList<String>();
        Log.i(TAG, "There are " + devices.size() + " USB devices available");
        if(devices.size() > 0) {
            deviceNames = new ArrayList<String>(devices.keySet());
            for(String deviceName : deviceNames) {
                Log.i(TAG, "Availble device: " + deviceName);
            }

        }
        setListAdapter(new ArrayAdapter<String>(this,
                    android.R.layout.simple_list_item_1,
                    deviceNames));
    }
}
