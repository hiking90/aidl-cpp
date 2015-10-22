/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.aidl.tests;

import android.app.Activity;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import java.util.concurrent.atomic.AtomicBoolean;

// Generated
import android.aidl.tests.ITestService;

public class TestServiceClient extends Activity {
    private static final String TAG = "TestServiceClient";
    private AtomicBoolean mKeepGoing = new AtomicBoolean(true);

    private Runnable mPingThread = new Runnable() {
        private static final int PERIOD_SECONDS = 5;

        private final ServiceManager mServiceManager = new ServiceManager();
        private ITestService mService;

        public void run() {
            while (mKeepGoing.get()) {
                checkService();
                try {
                    Thread.sleep(PERIOD_SECONDS * 1000);
                } catch (InterruptedException e) { }
            }
        }

        public void checkService() {
            if (mService == null) {
                IBinder service = mServiceManager.getService(
                        ITestService.class.getName());
                if (service == null) {
                    Log.i(TAG, "Failed to obtain binder...");
                    return;
                }
                mService = ITestService.Stub.asInterface(service);
                if (mService == null) {
                    Log.wtf(TAG, "Failed to cast IBinder instance.");
                    return;
                }
            }
            try {
                int query = 8;  // extremely lucky value
                Log.i(TAG, "Querying with token=" + query);
                int response = mService.Ping(query);
                Log.i(TAG, "Got response=" + response);
            } catch (RemoteException ex) {
                Log.e(TAG, "Ping failed.");
                mService = null;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "TestServiceClient.onCreate()");
        // Keep the main thread free for event handling.
        new Thread(mPingThread).start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "TestServiceClient.onDestroy()");
        mKeepGoing.set(false);
    }
}
