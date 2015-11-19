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

import android.aidl.tests.SimpleParcelable;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

// Generated
import android.aidl.tests.ITestService;
import android.aidl.tests.INamedCallback;

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

    private void checkPrimitiveRepeat(ITestService service)
            throws TestFailException {
        mLog.log("Checking that service can repeat primitives back...");
        try {
            {
                boolean query = true;
                boolean response = service.RepeatBoolean(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            {
                char query = 'A';
                char response = service.RepeatChar(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            {
                byte query = -128;
                byte response = service.RepeatByte(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            {
                int query = 1 << 30;
                int response = service.RepeatInt(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            {
                long query = 1 << 60;
                long response = service.RepeatLong(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            {
                float query = 1.0f/3.0f;
                float response = service.RepeatFloat(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            {
                double query = 1.0/3.0;
                double response = service.RepeatDouble(query);
                if (query != response) {
                    mLog.logAndThrow("Repeat with " + query +
                                     " responded " + response);
                }
            }
            for (String query : Arrays.asList("not empty", "", "\0")) {
                String response = service.RepeatString(query);
                if (!query.equals(response)) {
                    mLog.logAndThrow("Repeat request with '" + query + "'" +
                                     " of length " + query.length() +
                                     " responded with '" + response + "'" +
                                     " of length " + response.length());
                }
            }
        } catch (RemoteException ex) {
            mLog.log(ex.toString());
            mLog.logAndThrow("Service failed to repeat a primitive back.");
        }
        mLog.log("...Basic primitive repeating works.");
    }

    private void checkNullHandling(ITestService service)
            throws TestFailException {
        mLog.log("Checking that sending null strings reports an error...");
        try {
            String response = service.RepeatString(null);
            mLog.logAndThrow("Expected to fail on null string input!");
        } catch (NullPointerException ex) {
            mLog.log("Caught an exception on null string parameter (expected)");
            mLog.log("null strings behave as expected");
            return;
        } catch (Exception ex) {
            mLog.logAndThrow("Expected to receive NullPointerException on " +
                             "null parameter, but got " + ex.toString());
        }
        mLog.logAndThrow("Expected to receive NullPointerException on " +
                         "null parameter, but nothing was thrown??");
    }

    private void checkArrayReversal(ITestService service)
            throws TestFailException {
        mLog.log("Checking that service can reverse and return arrays...");
        try {
            {
                boolean[] input = {true, false, false, false};
                boolean echoed[] = new boolean[input.length];
                boolean[] reversed = service.ReverseBoolean(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                byte[] input = {0, 1, 2};
                byte echoed[] = new byte[input.length];
                byte[] reversed = service.ReverseByte(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                char[] input = {'A', 'B', 'C', 'D', 'E'};
                char echoed[] = new char[input.length];
                char[] reversed = service.ReverseChar(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                int[] input = {-1, 0, 1, 2, 3, 4, 5, 6};
                int echoed[] = new int[input.length];
                int[] reversed = service.ReverseInt(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                long[] input = {-2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8};
                long echoed[] = new long[input.length];
                long[] reversed = service.ReverseLong(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                float[] input = {0.0f, 1.0f, -0.3f};
                float echoed[] = new float[input.length];
                float[] reversed = service.ReverseFloat(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                double[] input = {-1.0, -4.0, -2.0};
                double echoed[] = new double[input.length];
                double[] reversed = service.ReverseDouble(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (input[i] != reversed[j]) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
            {
                String[] input = {"For", "relaxing", "times"};
                String echoed[] = new String[input.length];
                String[] reversed = service.ReverseString(input, echoed);
                if (!Arrays.equals(input, echoed)) {
                    mLog.logAndThrow("Failed to echo input array back.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow("Reversed array is the wrong size.");
                }
                for (int i = 0; i < input.length; ++i) {
                    int j = reversed.length - (1 + i);
                    if (!input[i].equals(reversed[j])) {
                        mLog.logAndThrow(
                                "input[" + i + "] = " + input[i] +
                                " but reversed value = " + reversed[j]);
                    }
                }
            }
        } catch (RemoteException ex) {
            mLog.log(ex.toString());
            mLog.logAndThrow("Service failed to reverse an array.");
        }
        mLog.log("...service can reverse and return arrays.");
    }

    private void checkBinderExchange(
                ITestService service) throws TestFailException {
      mLog.log("Checking exchange of binders...");
      try {
          INamedCallback got = service.GetOtherTestService("Smythe");
          mLog.log("Received test service");
          String name = got.GetName();

          if (!name.equals("Smythe")) {
              mLog.logAndThrow("Tried to get service with name 'Smythe'" +
                               " and found service with name '" + name + "'");
          }

          if (!service.VerifyName(got, "Smythe")) {
              mLog.logAndThrow("Test service could not verify name of 'Smythe'");
          }
      } catch (RemoteException ex) {
          mLog.log(ex.toString());
          mLog.logAndThrow("Service failed to exchange binders.");
      }
      mLog.log("...Exchange of binders works");
    }

    private void checkListReversal(ITestService service)
            throws TestFailException {
        mLog.log("Checking that service can reverse and return lists...");
        try {
            {
                List<String> input = Arrays.asList("Walk", "into", "Córdoba");
                List<String> echoed = new ArrayList<String>();
                List<String> reversed = service.ReverseStringList(input, echoed);
                if (!input.equals(echoed)) {
                    mLog.logAndThrow("Failed to echo input List<String> back.");
                }
                Collections.reverse(input);
                if (!input.equals(reversed)) {
                    mLog.logAndThrow("Reversed list is not correct.");
                }
            }
        } catch (RemoteException ex) {
            mLog.log(ex.toString());
            mLog.logAndThrow("Service failed to reverse an List<String>.");
        }
        mLog.log("...service can reverse and return lists.");
    }

    private void checkParcelables(ITestService service)
            throws TestFailException {
        mLog.log("Checking that service can repeat and reverse parcelables...");
        try {
            {
                SimpleParcelable input = new SimpleParcelable("foo", 42);
                SimpleParcelable out_param = new SimpleParcelable();
                SimpleParcelable returned =
                        service.RepeatParcelable(input, out_param);
                if (!input.equals(out_param)) {
                    mLog.log(input.toString() + " != " + out_param.toString());
                    mLog.logAndThrow("out param parcelable was not equivalent");
                }
                if (!input.equals(returned)) {
                    mLog.log(input.toString() + " != " + returned.toString());
                    mLog.logAndThrow("returned parcelable was not equivalent");
                }
            }
            {
                SimpleParcelable[] input = new SimpleParcelable[3];
                input[0] = new SimpleParcelable("a", 1);
                input[1] = new SimpleParcelable("b", 2);
                input[2] = new SimpleParcelable("c", 3);
                SimpleParcelable[] repeated = new SimpleParcelable[3];
                SimpleParcelable[] reversed = service.ReverseParcelables(
                        input, repeated);
                if (!Arrays.equals(input, repeated)) {
                    mLog.logAndThrow(
                            "Repeated list of parcelables did not match.");
                }
                if (input.length != reversed.length) {
                    mLog.logAndThrow(
                            "Reversed list of parcelables had wrong length.");
                }
                for (int i = 0, k = input.length - 1;
                     i < input.length;
                     ++i, --k) {
                    if (!input[i].equals(reversed[k])) {
                        mLog.log(input[i].toString() + " != " +
                                 reversed[k].toString());
                        mLog.logAndThrow("reversed parcelable was " +
                                         "not equivalent");
                    }
                }
            }
        } catch (Exception ex) {
            mLog.log(ex.toString());
            mLog.logAndThrow("Service failed to handle Parcelables.");
        }
        mLog.log("...service can manipulate parcelables.");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Starting!");
        try {
          init();
          ITestService service = getService();
          checkPrimitiveRepeat(service);
          checkNullHandling(service);
          checkArrayReversal(service);
          checkBinderExchange(service);
          checkListReversal(service);
          checkParcelables(service);
          mLog.log(mSuccessSentinel);
        } catch (TestFailException e) {
            mLog.log(mFailureSentinel);
            throw new RuntimeException(e);
        } finally {
            if (mLog != null) {
                mLog.close();
            }
        }
    }
}
