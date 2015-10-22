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

#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

#include "android/aidl/tests/BnTestService.h"
#include "android/aidl/tests/ITestService.h"

// Used implicitly.
#undef LOG_TAG
#define LOG_TAG "aidl_native_service"

// libutils:
using android::Looper;
using android::LooperCallback;
using android::OK;
using android::sp;
using android::status_t;
using android::String16;

// libbinder:
using android::BnInterface;
using android::defaultServiceManager;
using android::IInterface;
using android::IPCThreadState;
using android::Parcel;
using android::ProcessState;

// Generated code:
using android::aidl::tests::BnTestService;

namespace android {
namespace generated {
namespace {

class BinderCallback : public LooperCallback {
 public:
  BinderCallback() {}
  ~BinderCallback() override {}

  int handleEvent(int /* fd */, int /* events */, void* /* data */ ) override {
    IPCThreadState::self()->handlePolledCommands();
    return 1;  // Continue receiving callbacks.
  }
};

class NativeService : public BnTestService {
 public:
  NativeService() {}
  ~NativeService() override {}

  int Run() {
    sp<Looper> looper(Looper::prepare(0 /* opts */));

    int binder_fd = -1;
    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    IPCThreadState::self()->disableBackgroundScheduling(true);
    IPCThreadState::self()->setupPolling(&binder_fd);
    ALOGI("Got binder FD %d", binder_fd);
    if (binder_fd < 0) return -1;

    sp<BinderCallback> cb(new BinderCallback);
    if (looper->addFd(binder_fd, Looper::POLL_CALLBACK, Looper::EVENT_INPUT, cb,
                      nullptr) != 1) {
      ALOGE("Failed to add binder FD to Looper");
      return -1;
    }

    defaultServiceManager()->addService(getInterfaceDescriptor(), this);

    ALOGI("Entering loop");
    while (true) {
      const int result = looper->pollAll(-1 /* timeoutMillis */);
      ALOGI("Looper returned %d", result);
    }
    return 0;
  }

  // BnTestService:
  status_t Ping(int32_t token, int32_t* returned_token) override {
    ALOGI("Got ping with token %d", token);
    *returned_token = token;
    return OK;
  }
};

}  // namespace
}  // namespace generated
}  // namespace android

int main(int /* argc */, char* /* argv */ []) {
  android::generated::NativeService service;
  return service.Run();
}
