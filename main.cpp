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

#include <iostream>
#include <memory>

#include "aidl.h"
#include "options.h"

using android::aidl::Options;

int main(int argc, const char** argv) {
  std::unique_ptr<Options> options = Options::ParseOptions(argc, argv);
  if (!options) {
    return 1;
  }

  switch (options->task) {
    case Options::COMPILE_AIDL_TO_JAVA:
      return android::aidl::compile_aidl(*options);
    case Options::PREPROCESS_AIDL:
      return android::aidl::preprocess_aidl(*options);
  }
  std::cerr << "aidl: internal error" << std::endl;
  return 1;
}
