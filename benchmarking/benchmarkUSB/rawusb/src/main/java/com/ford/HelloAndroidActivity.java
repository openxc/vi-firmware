package com.ford;

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
        setListAdapter(new ArrayAdapter<String>(this,
                    R.layout.simple_list_item_1, devices.keySet()));
    }
}
