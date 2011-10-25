package com.ford;

import java.util.concurrent.TimeUnit;

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

import android.R.id;

import android.util.Log;

import android.widget.TextView;

public class SteeringWheelDisplay extends Activity {

    private static String TAG = "rawusb";

    private final Handler mHandler = new Handler();
    private UsbManager mUsbManager;

    private TextView mDeviceNameView;
    private TextView mInterfaceCountView;
    private TextView mEndpointCountView;
    private TextView mTransferredBytesView;
    private TextView mElapsedTimeView;
    private TextView mTransferRateView;

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
        mElapsedTimeView = (TextView) findViewById(R.id.elapsed_time);
        mTransferRateView = (TextView) findViewById(R.id.transfer_rate);

        mElapsedTimeView.setText("0");
    }

    @Override
    public void onResume() {
        super.onResume();
        mHandler.post(mSetupDeviceTask);
    }

    @Override
    public void onPause() {
        super.onPause();
        mHandler.removeCallbacks(mSetupDeviceTask);
    }

    private void transferData() {
        byte[] bytes = new byte[2048];
        int transferred = 0;
        final long startTime = System.nanoTime();
        final long endTime;
        while(transferred < 1000 * 1000) {
            transferred += mConnection.bulkTransfer(mEndpoint, bytes,
                bytes.length, 0);
            final int currentTransferred = transferred;
            mHandler.post(new Runnable() {
                public void run() {
                    double kilobytesTransferred = currentTransferred / 1000.0;
                    mTransferredBytesView.setText(
                        Double.toString(kilobytesTransferred));
                    long elapsedTime = TimeUnit.SECONDS.convert(
                        System.nanoTime() - startTime, TimeUnit.NANOSECONDS);
                    mElapsedTimeView.setText(elapsedTime + " seconds");
                    mTransferRateView.setText(
                        kilobytesTransferred / elapsedTime + " KB/s");
                }
            });
        }
        endTime = System.nanoTime();
        Log.i(TAG, "Transferred " + transferred + " bytes in "
            + TimeUnit.SECONDS.convert(endTime - startTime,
                TimeUnit.NANOSECONDS)
            + " seconds");
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

            new Thread(new Runnable() {
                public void run() {
                    transferData();
                }
            }).start();
        }
    };
}
