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

#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

#include "android/aidl/tests/ITestService.h"

// libutils:
using android::OK;
using android::sp;
using android::status_t;
using android::String16;
using android::String8;

// libbinder:
using android::getService;

// generated
using android::aidl::tests::ITestService;

using std::cerr;
using std::cout;
using std::endl;
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
                     status_t(ITestService::*func)(T, T*),
                     const T input) {
  T reply;
  status_t status = (*service.*func)(input, &reply);
  if (status != OK || input != reply) {
    cerr << "Failed to repeat primitive. status=" << status << "." << endl;
    return false;
  }
  return true;
}

bool ConfirmPrimitiveRepeat(const sp<ITestService>& s) {
  cout << "Confirming passing and returning primitives." << endl;

  if (!RepeatPrimitive(s, &ITestService::RepeatBoolean, true) ||
      !RepeatPrimitive(s, &ITestService::RepeatByte, int8_t{-128}) ||
      !RepeatPrimitive(s, &ITestService::RepeatChar, char16_t{'A'}) ||
      !RepeatPrimitive(s, &ITestService::RepeatInt, int32_t{1 << 30}) ||
      !RepeatPrimitive(s, &ITestService::RepeatLong, int64_t{1ll << 60}) ||
      !RepeatPrimitive(s, &ITestService::RepeatFloat, float{1.0f/3.0f}) ||
      !RepeatPrimitive(s, &ITestService::RepeatDouble, double{1.0/3.0})) {
    return false;
  }

  vector<String16> inputs = {
      String16("Deliver us from evil."),
      String16(),
      String16("\0\0", 2),
  };
  for (const auto& input : inputs) {
    String16 reply;
    status_t status = s->RepeatString(input, &reply);
    if (status != OK || input != reply) {
      cerr << "Failed while requesting service to repeat String16=\""
           << String8(input).string()
           << "\". Got status=" << status << endl;
      return false;
    }
  }
  return true;
}

}  // namespace

int main(int /* argc */, char * /* argv */ []) {
  sp<ITestService> service;

  if (!GetService(&service)) return 1;

  if (!ConfirmPrimitiveRepeat(service)) return 1;

  return 0;
}
