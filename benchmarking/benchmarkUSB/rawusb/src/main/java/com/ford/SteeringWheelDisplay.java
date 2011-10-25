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
import android.os.Handler;

import android.util.Log;

import android.widget.TextView;

public class SteeringWheelDisplay extends Activity {

    private static String TAG = "rawusb";
    private static final String ACTION_USB_PERMISSION =
            "com.ford.rawusb.USB_PERMISSION";

    private final Handler mHandler = new Handler();
    private UsbManager mUsbManager;

    private TextView mDeviceNameView;
    private TextView mInterfaceCountView;
    private TextView mEndpointCountView;
    private TextView mTransferredBytesView;

    private UsbDeviceConnection mConnection;
    private UsbEndpoint mEndpoint;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.detail);
		Log.i(TAG, "SteeringWheelDisplay created");
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        mDeviceNameView = (TextView) findViewById(R.id.device_name);
        mInterfaceCountView = (TextView) findViewById(R.id.interface_count);
        mEndpointCountView = (TextView) findViewById(R.id.endpoint_count);
        mTransferredBytesView = (TextView) findViewById(R.id.transfer_total);
    }

    @Override
    public void onResume() {
        super.onResume();
        mHandler.post(mSetupDeviceTask);
    }

    private final Runnable mSetupDeviceTask = new Runnable() {
        public void run() {
            String deviceName =
                getIntent().getStringExtra("usb_device_name");
            Log.i(TAG, "Clicked " + deviceName);

            mDeviceNameView.setText(deviceName);

            Map<String, UsbDevice> devices = mUsbManager.getDeviceList();
            UsbDevice device = devices.get(deviceName);
            mInterfaceCountView.setText(
                    Integer.toString(device.getInterfaceCount()));
            UsbInterface iface = device.getInterface(0);
            mEndpointCountView.setText(
                    Integer.toString(iface.getEndpointCount()));
            mEndpoint = iface.getEndpoint(1);
            mConnection = mUsbManager.openDevice(device);
            mConnection.claimInterface(iface, true);
            mHandler.post(mTransferDataTask);
        }
    };

    private final Runnable mTransferDataTask = new Runnable() {
        public void run() {
            byte[] bytes = new byte[64];
            int transferred = 0;
            while(transferred < 1000 * 1000) {
                transferred += mConnection.bulkTransfer(mEndpoint, bytes,
                    bytes.length, 0);
                mTransferredBytesView.setText(Integer.toString(transferred));
            }
            Log.i(TAG, "Transferred " + transferred + " bytes");
        }
    };
}
