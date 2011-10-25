package com.ford;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;

import android.app.ListActivity;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;

import android.view.View;

import android.widget.AdapterView;

import android.widget.AdapterView.OnItemClickListener;

import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class UsbTestActivity extends ListActivity {

    private static String TAG = "rawusb";
    private static final String ACTION_USB_PERMISSION =
            "com.ford.rawusb.USB_PERMISSION";

    private UsbManager mUsbManager;

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (!ACTION_USB_PERMISSION.equals(action)) {
                Log.i(TAG, "Refreshing USB devices as something was " +
                        "plugged/unplugged");
                UsbTestActivity.this.refreshDeviceList();
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		Log.i(TAG, "onCreate");

        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        IntentFilter filter = new IntentFilter();
        filter.addAction("android.hardware.usb.action.USB_DEVICE_ATTACHED");
        filter.addAction("android.hardware.usb.action.USB_DEVICE_DETACHED");
        registerReceiver(mBroadcastReceiver, filter);

        refreshDeviceList();

        ListView view = getListView();
        view.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view,
                    int position, long id) {
                Intent intent = new Intent(UsbTestActivity.this,
                    SteeringWheelDisplay.class);
                intent.putExtra("usb_device_name",
                    ((TextView) view).getText().toString());
                startActivity(intent);
            }
        });
    }

    public void refreshDeviceList() {
        Map<String, UsbDevice> devices = mUsbManager.getDeviceList();

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
