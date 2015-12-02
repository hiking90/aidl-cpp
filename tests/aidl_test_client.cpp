/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <vector>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <nativehelper/ScopedFd.h>
#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

#include "android/aidl/tests/ITestService.h"
#include "android/aidl/tests/INamedCallback.h"

// libutils:
using android::OK;
using android::sp;
using android::status_t;
using android::String16;
using android::String8;

// libbinder:
using android::getService;
using android::IBinder;
using android::binder::Status;

// generated
using android::aidl::tests::ITestService;
using android::aidl::tests::INamedCallback;
using android::aidl::tests::SimpleParcelable;

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace {

const char kServiceName[] = "android.aidl.tests.ITestService";

bool GetService(sp<ITestService>* service) {
  cout << "Retrieving test service binder" << endl;
  status_t status = getService(String16(kServiceName), service);
  if (status != OK) {
    cerr << "Failed to get service binder: '" << kServiceName
         << "' status=" << status << endl;
    return false;
  }
  return true;
}

template <typename T>
bool RepeatPrimitive(const sp<ITestService>& service,
                     Status(ITestService::*func)(T, T*),
                     const T input) {
  T reply;
  Status status = (*service.*func)(input, &reply);
  if (!status.isOk() || input != reply) {
    cerr << "Failed to repeat primitive. status=" << status.toString8()
         << "." << endl;
    return false;
  }
  return true;
}

bool ConfirmPrimitiveRepeat(const sp<ITestService>& s) {
  cout << "Confirming passing and returning primitives works." << endl;

  if (!RepeatPrimitive(s, &ITestService::RepeatBoolean, true) ||
      !RepeatPrimitive(s, &ITestService::RepeatByte, int8_t{-128}) ||
      !RepeatPrimitive(s, &ITestService::RepeatChar, char16_t{'A'}) ||
      !RepeatPrimitive(s, &ITestService::RepeatInt, int32_t{1 << 30}) ||
      !RepeatPrimitive(s, &ITestService::RepeatLong, int64_t{1ll << 60}) ||
      !RepeatPrimitive(s, &ITestService::RepeatFloat, float{1.0f/3.0f}) ||
      !RepeatPrimitive(s, &ITestService::RepeatDouble, double{1.0/3.0}) ||
      !RepeatPrimitive(s, &ITestService::RepeatInt, ITestService::TEST_CONSTANT)) {
    return false;
  }

  vector<String16> inputs = {
      String16("Deliver us from evil."),
      String16(),
      String16("\0\0", 2),
  };
  for (const auto& input : inputs) {
    String16 reply;
    Status status = s->RepeatString(input, &reply);
    if (!status.isOk() || input != reply) {
      cerr << "Failed while requesting service to repeat String16=\""
           << String8(input).string()
           << "\". Got status=" << status.toString8() << endl;
      return false;
    }
  }
  return true;
}

template <typename T>
bool ReverseArray(const sp<ITestService>& service,
                  Status(ITestService::*func)(const vector<T>&,
                                              vector<T>*,
                                              vector<T>*),
                  vector<T> input) {
  vector<T> actual_reversed;
  vector<T> actual_repeated;
  Status status = (*service.*func)(input, &actual_repeated, &actual_reversed);
  if (!status.isOk()) {
    cerr << "Failed to repeat array. status=" << status.toString8() << "."
         << endl;
    return false;
  }
  if (input != actual_repeated) {
    cerr << "Repeated version of array did not match" << endl;
    cerr << "input.size()=" << input.size()
         << " repeated.size()=" << actual_repeated.size() << endl;
    return false;
  }
  std::reverse(input.begin(), input.end());
  if (input != actual_reversed) {
    cerr << "Reversed version of array did not match" << endl;
    return false;
  }
  return true;
}

bool ConfirmReverseArrays(const sp<ITestService>& s) {
  cout << "Confirming passing and returning arrays works." << endl;

  if (!ReverseArray(s, &ITestService::ReverseBoolean,
                    {true, false, false}) ||
      !ReverseArray(s, &ITestService::ReverseByte,
                    {int8_t{-128}, int8_t{0}, int8_t{127}}) ||
      !ReverseArray(s, &ITestService::ReverseChar,
                    {char16_t{'A'}, char16_t{'B'}, char16_t{'C'}}) ||
      !ReverseArray(s, &ITestService::ReverseInt,
                    {1, 2, 3}) ||
      !ReverseArray(s, &ITestService::ReverseLong,
                    {-1ll, 0ll, int64_t{1ll << 60}}) ||
      !ReverseArray(s, &ITestService::ReverseFloat,
                    {-0.3f, -0.7f, 8.0f}) ||
      !ReverseArray(s, &ITestService::ReverseDouble,
                    {1.0/3.0, 1.0/7.0, 42.0}) ||
      !ReverseArray(s, &ITestService::ReverseString,
                    {String16{"f"}, String16{"a"}, String16{"b"}})) {
    return false;
  }

  return true;
}

bool ConfirmReverseLists(const sp<ITestService>& s) {
  cout << "Confirming passing and returning List<T> works." << endl;

  if (!ReverseArray(s, &ITestService::ReverseStringList,
                    {String16{"f"}, String16{"a"}, String16{"b"}})) {
    return false;
  }

  return true;
}

bool ConfirmParcelables(const sp<ITestService>& s) {
  cout << "Confirming passing and returning Parcelables works." << endl;

  SimpleParcelable input("Booya", 42);
  SimpleParcelable out_param, returned;
  Status status = s->RepeatParcelable(input, &out_param, &returned);
  if (!status.isOk()) {
    cout << "Binder call failed." << endl;
    return false;
  }
  if (input != out_param || input != returned) {
    cout << "Failed to repeat parcelables." << endl;
    return false;
  }

  cout << "Attempting to reverse an array of parcelables." << endl;
  const vector<SimpleParcelable> original{SimpleParcelable("first", 0),
                                          SimpleParcelable("second", 1),
                                          SimpleParcelable("third", 2)};
  vector<SimpleParcelable> repeated;
  vector<SimpleParcelable> reversed;
  status = s->ReverseParcelables(original, &repeated, &reversed);
  if (!status.isOk()) {
    cout << "Binder call failed." << endl;
    return false;
  }
  std::reverse(reversed.begin(), reversed.end());
  if (repeated != original || reversed != original) {
    cout << "Failed to reverse an array of parcelables." << endl;
    return false;
  }

  return true;
}

bool ConfirmReverseBinderLists(const sp<ITestService>& s) {
  Status status;
  cout << "Confirming passing and returning List<T> works with binders." << endl;

  vector<String16> names = {
    String16{"Larry"},
    String16{"Curly"},
    String16{"Moe"}
  };

  vector<sp<IBinder>> input;

  for (int i = 0; i < 3; i++) {
    sp<INamedCallback> got;

    status = s->GetOtherTestService(names[i], &got);
    if (!status.isOk()) {
      cerr << "Could not retrieve service for test." << endl;
      return false;
    }

    input.push_back(INamedCallback::asBinder(got));
  }

  vector<sp<IBinder>> output;
  vector<sp<IBinder>> reversed;

  status = s->ReverseNamedCallbackList(input, &output, &reversed);
  if (!status.isOk()) {
    cerr << "Failed to reverse named callback list." << endl;
  }

  if (output.size() != 3) {
    cerr << "ReverseNamedCallbackList gave repetition with wrong length." << endl;
    return false;
  }

  if (reversed.size() != 3) {
    cerr << "ReverseNamedCallbackList gave reversal with wrong length." << endl;
    return false;
  }

  for (int i = 0; i < 3; i++) {
    String16 ret;
    sp<INamedCallback> named_callback =
        android::interface_cast<INamedCallback>(output[i]);
    status = named_callback->GetName(&ret);

    if (!status.isOk()) {
      cerr << "Could not query INamedCallback from output" << endl;
      return false;
    }

    if (ret != names[i]) {
      cerr << "Output had wrong INamedCallback" << endl;
      return false;
    }
  }

  for (int i = 0; i < 3; i++) {
    String16 ret;
    sp<INamedCallback> named_callback =
        android::interface_cast<INamedCallback>(reversed[i]);
    status = named_callback->GetName(&ret);

    if (!status.isOk()) {
      cerr << "Could not query INamedCallback from reversed output" << endl;
      return false;
    }

    if (ret != names[2 - i]) {
      cerr << "Reversed output had wrong INamedCallback" << endl;
      return false;
    }
  }

  return true;
}

#define FdByName(_fd) #_fd, _fd

bool DoWrite(string name, const ScopedFd& fd, const string& buf) {
  int wrote;

  while ((wrote = write(fd.get(), buf.data(), buf.size())) < 0 && errno == EINTR);

  if (wrote == (signed)buf.size()) {
    return true;
  }

  if (wrote < 0) {
    cerr << "Error writing to file descriptor '" << name << "': "
        << strerror(errno) << endl;
  } else {
    cerr << "File descriptor '" << name << "'accepted short data." << endl;
  }

  return false;
}

bool DoRead(string name, const ScopedFd& fd, const string& expected) {
  size_t length = expected.size();
  int got;
  string buf;
  buf.resize(length);

  while ((got = read(fd.get(), &buf[0], length)) < 0 && errno == EINTR);

  if (got < 0) {
    cerr << "Error reading from '" << name << "': " << strerror(errno) << endl;
    return false;
  }

  if (buf != expected) {
    cerr << "Expected '" << expected << "' got '" << buf << "'" << endl;
    return false;
  }

  return true;
}

bool DoPipe(ScopedFd* read_side, ScopedFd* write_side) {
  int fds[2];
  ScopedFd return_fd;

  if (pipe(fds)) {
    cout << "Error creating pipes: " << strerror(errno) << endl;
    return false;
  }

  read_side->reset(fds[0]);
  write_side->reset(fds[1]);
  return true;
}

bool ConfirmFileDescriptors(const sp<ITestService>& s) {
  Status status;
  cout << "Confirming passing and returning file descriptors works." << endl;

  ScopedFd return_fd;
  ScopedFd read_fd;
  ScopedFd write_fd;

  if (!DoPipe(&read_fd, &write_fd)) {
    return false;
  }

  status = s->RepeatFileDescriptor(write_fd, &return_fd);

  if (!status.isOk()) {
    cerr << "Could not repeat file descriptors." << endl;
    return false;
  }

  /* A note on some of the spookier stuff going on here: IIUC writes to pipes
   * should be atomic and non-blocking so long as the total size doesn't exceed
   * PIPE_BUF. We thus play a bit fast and loose with failure modes here.
   */

  bool ret =
      DoWrite(FdByName(return_fd), "ReturnString") &&
      DoRead(FdByName(read_fd), "ReturnString");

  return ret;
}

bool ConfirmFileDescriptorArrays(const sp<ITestService>& s) {
  Status status;
  cout << "Confirming passing and returning file descriptor arrays works." << endl;

  vector<ScopedFd> array;
  array.resize(2);

  if (!DoPipe(&array[0], &array[1])) {
    return false;
  }

  vector<ScopedFd> repeated;
  vector<ScopedFd> reversed;

  status = s->ReverseFileDescriptorArray(array, &repeated, &reversed);

  if (!status.isOk()) {
    cerr << "Could not reverse file descriptor array." << endl;
    return false;
  }

  bool ret =
      DoWrite(FdByName(array[1]), "First") &&
      DoWrite(FdByName(repeated[1]), "Second") &&
      DoWrite(FdByName(reversed[0]), "Third") &&
      DoRead(FdByName(reversed[1]), "FirstSecondThird");

  return ret;
}
}  // namespace

int main(int /* argc */, char * /* argv */ []) {
  sp<ITestService> service;

  if (!GetService(&service)) return 1;

  if (!ConfirmPrimitiveRepeat(service)) return 1;

  if (!ConfirmReverseArrays(service)) return 1;

  if (!ConfirmReverseLists(service)) return 1;

  if (!ConfirmParcelables(service)) return 1;

  if (!ConfirmReverseBinderLists(service)) return 1;

  if (!ConfirmFileDescriptors(service)) return 1;

  if (!ConfirmFileDescriptorArrays(service)) return 1;

  return 0;
}
