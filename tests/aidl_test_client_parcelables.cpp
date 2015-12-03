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

#include "aidl_test_client_parcelables.h"

#include <iostream>
#include <vector>

// libutils:
using android::sp;

// libbinder:
using android::binder::Status;

// generated
using android::aidl::tests::ITestService;
using android::aidl::tests::SimpleParcelable;

using std::cout;
using std::endl;
using std::vector;

namespace android {
namespace aidl {
namespace tests {
namespace client {

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

}  // namespace client
}  // namespace tests
}  // namespace aidl
}  // namespace android
