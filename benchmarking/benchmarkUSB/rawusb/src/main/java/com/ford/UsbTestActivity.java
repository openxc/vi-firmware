package com.ford;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;

import android.app.Activity;
import android.app.ListActivity;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

import android.widget.ArrayAdapter;

public class UsbTestActivity extends ListActivity {

    private static String TAG = "rawusb";
    private Handler mHandler;
    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG, "Refreshing USB devices as something was " +
                    "plugged/unplugged");
            UsbTestActivity.this.refreshDeviceList();
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		Log.i(TAG, "onCreate");

        IntentFilter filter = new IntentFilter();
        filter.addAction("android.hardware.usb.action.USB_DEVICE_ATTACHED");
        filter.addAction("android.hardware.usb.action.USB_DEVICE_DETACHED");
        registerReceiver(mBroadcastReceiver, filter);

        mHandler = new Handler();
        refreshDeviceList();
    }

    public void refreshDeviceList() {
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        Map<String, UsbDevice> devices = manager.getDeviceList();

        List<String> deviceNames = new ArrayList<String>();
        Log.i(TAG, "There are " + devices.size() + " USB devices available");
        if(devices.size() > 0) {
            deviceNames = new ArrayList<String>(devices.keySet());
            for(String deviceName : deviceNames) {
                Log.i(TAG, "Available USB device: " + deviceName);
            }

        }
        setListAdapter(new ArrayAdapter<String>(this,
                    android.R.layout.simple_list_item_1,
                    deviceNames));
    }
}
