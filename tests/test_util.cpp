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

#include "tests/test_util.h"

using base::FilePath;
using std::string;

namespace android {
namespace aidl {
namespace test {

FilePath CanonicalNameToPath(const char* package_class, const char* extension) {
  string rel_path{package_class};
  for (char& c : rel_path) {
    if (c == '.') {
      c = FilePath::kSeparators[0];
    }
  }
  rel_path += extension;
  return FilePath(rel_path);
}

void SplitPackageClass(const string& package_class,
                       FilePath* rel_path,
                       string* package,
                       string* class_name) {
  *package = string{package_class, 0, package_class.rfind('.')};
  *class_name = string{package_class, package_class.rfind('.') + 1};
  *rel_path = CanonicalNameToPath(package_class.c_str(), ".aidl");
}

}  // namespace test
}  // namespace android
}  // namespace aidl
