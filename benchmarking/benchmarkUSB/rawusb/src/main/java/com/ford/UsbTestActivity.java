package com.ford;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;

import android.app.Activity;
import android.app.ListActivity;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;

import android.view.View;

import android.widget.AdapterView;

import android.widget.AdapterView.OnItemClickListener;

import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class UsbTestActivity extends ListActivity {

    private static final String ACTION_USB_PERMISSION =
            "com.ford.rawusb.USB_PERMISSION";
    private static String TAG = "rawusb";
    private UsbManager mUsbManager;

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = (UsbDevice)intent.getParcelableExtra(
                            UsbManager.EXTRA_DEVICE);

                    if (intent.getBooleanExtra(
                                UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if(device != null){
                            //call method to set up device communication
                        }
                    }
                    else {
                        Log.d(TAG, "permission denied for device " + device);
                    }
                }
            } else {
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

                String deviceName = ((TextView) view).getText().toString();
                Log.i(TAG, "Clicked " + deviceName);

                Map<String, UsbDevice> devices = mUsbManager.getDeviceList();
                UsbDevice device = devices.get(deviceName);
                Log.i(TAG, "Device has " + device.getInterfaceCount() +
                    "interfaces");
                UsbInterface iface = device.getInterface(0);
                Log.i(TAG, "Device has " + iface.getEndpointCount() +
                    "endpoints");
                UsbEndpoint endpoint = iface.getEndpoint(1);
                UsbDeviceConnection connection = mUsbManager.openDevice(
                    device);
                connection.claimInterface(iface, true);

                byte[] bytes = new byte[64];
                int transferred = connection.bulkTransfer(endpoint, bytes,
                    bytes.length, 0);
                Log.i(TAG, "Transferred " + transferred + " bytes");
                if(transferred > 0) {
                    Log.i(TAG, "Data: " + new String(bytes));
                }

                connection.controlTransfer(0x40, 0x80, 42, 0, null, 0, 0);
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
