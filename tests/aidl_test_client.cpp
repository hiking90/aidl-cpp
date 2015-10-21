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

#include <binder/IServiceManager.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>

#include "android/os/IPingResponder.h"

// libutils:
using android::OK;
using android::sp;
using android::status_t;
using android::String16;

// libbinder:
using android::getService;

// generated
using android::os::IPingResponder;

using std::cerr;
using std::cout;
using std::endl;

namespace {

const char kServiceName[] = "android.os.IPingResponder";

bool ConfirmBasicPing(const sp<IPingResponder>& service) {
  cout << "Confirming basic ping functionality." << endl;
  const int32_t kIncrement = 1 << 20;
  for (int32_t i = 0; i < 3; ++i) {
    const int32_t token = -kIncrement + i * kIncrement;
    int32_t reply = -1;
    if (service->Ping(token, &reply) != OK) {
      cerr << "Failed to ping server with token=" << token << endl;
      return false;
    }
    if (token != reply) {
      cerr << "Server replied to token=" << token
           << " with reply=" << reply << endl;
      return false;
    }
  }
  return true;
}

bool GetService(sp<IPingResponder>* service) {
  cout << "Retrieving ping service binder" << endl;
  status_t status = getService(String16(kServiceName), service);
  if (status != OK) {
    cerr << "Failed to get service binder: '" << kServiceName
         << "' status=" << status << endl;
    return false;
  }
  return true;
}

}  // namespace

int main(int /* argc */, char * /* argv */ []) {
  sp<IPingResponder> service;

  if (!GetService(&service)) return 1;

  if (!ConfirmBasicPing(service)) return 1;

  return 0;
}
