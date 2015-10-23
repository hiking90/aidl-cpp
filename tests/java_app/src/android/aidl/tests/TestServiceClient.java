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
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.FileOutputStream;

// Generated
import android.aidl.tests.ITestService;

public class TestServiceClient extends Activity {
    private static final String TAG = "TestServiceClient";

    public class TestFailException extends Exception {
        public TestFailException(String message) {
            super(message);
        }
    }

    private class Logger {
      private PrintWriter mLogFile;

      public Logger() {
        try {
            mLogFile = new PrintWriter(openFileOutput(
                    "test-client.log", Context.MODE_WORLD_READABLE));
        } catch (IOException ex) {
            throw new RuntimeException("Failed to open log file for writing.");
        }
      }

      public void log(String line) {
          Log.i(TAG, line);
          mLogFile.println(line);
      }

      public void logAndThrow(String line) throws TestFailException {
          Log.e(TAG, line);
          mLogFile.println(line);
          throw new TestFailException(line);
      }

      public void close() {
          if (mLogFile != null) {
              mLogFile.close();
          }
      }
    }


    private Logger mLog;
    private String mSuccessSentinel;
    private String mFailureSentinel;

    private void init() {
        Intent intent = getIntent();
        mLog = new Logger();
        mLog.log("Reading sentinels from intent...");
        mSuccessSentinel = intent.getStringExtra("sentinel.success");
        mFailureSentinel = intent.getStringExtra("sentinel.failure");
        if (mSuccessSentinel == null || mFailureSentinel == null) {
            String message = "Failed to read intent extra input.";
            Log.e(TAG, message);
            mLog.close();
            throw new RuntimeException(message);
        }
    }

    private ITestService getService() throws TestFailException {
        IBinder service = new ServiceManager().getService(
                ITestService.class.getName());
        if (service == null) {
            mLog.logAndThrow("Failed to obtain binder...");
        }
        ITestService ret = ITestService.Stub.asInterface(service);
        if (ret == null) {
            mLog.logAndThrow("Failed to cast IBinder instance.");
        }
        return ret;
    }

    private void checkBasicPing(ITestService service) throws TestFailException {
        mLog.log("Checking that basic ping works.");
        final int INCREMENT = 1 << 20;
        for (int i = 0; i < 3; ++i) {
            int query = -INCREMENT + i * INCREMENT;
            int response = -1;
            try {
                response = service.Ping(query);
            } catch (RemoteException ex) {
                mLog.log(ex.toString());
                mLog.logAndThrow("Failed pinging service with " + query);
            }
            if (query != response) {
                mLog.logAndThrow(
                        "Ping with " + query + " responded " + response);
            }
        }
        mLog.log("...Basic ping works.");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Starting!");
        try {
          init();
          ITestService service = getService();
          mLog.log(mSuccessSentinel);
        } catch (TestFailException e) {
            mLog.close();
            throw new RuntimeException(e);
        } finally {
            if (mLog != null) {
                mLog.close();
            }
        }
    }
}
