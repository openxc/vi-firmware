package com.ford;

import java.util.Map;

import android.app.Activity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import android.os.Bundle;

import android.util.Log;

public class SteeringWheelDisplay extends Activity {

    private static String TAG = "rawusb";
    private static final String ACTION_USB_PERMISSION =
            "com.ford.rawusb.USB_PERMISSION";

    private UsbManager mUsbManager;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		Log.i(TAG, "SteeringWheelDisplay created");
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        setupDevice(getIntent().getStringExtra("usb_device_name"));
    }

    private void setupDevice(String deviceName) {
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
            }
        }
    };
}
