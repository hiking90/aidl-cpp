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

#include "fake_io_delegate.h"

using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace test {

unique_ptr<string> FakeIoDelegate::GetFileContents(
    const string& filename,
    const string& content_suffix) const {
  unique_ptr<string> contents;
  auto it = file_contents_.find(filename);
  if (it == file_contents_.end()) {
    return contents;
  }
  contents.reset(new string);
  *contents = it->second;
  contents->append(content_suffix);

  return contents;
}

bool FakeIoDelegate::FileIsReadable(const std::string& path) const {
  return file_contents_.find(path) != file_contents_.end();
}

void FakeIoDelegate::SetFileContents(const string& filename,
                                     const string& contents) {
  file_contents_[filename] = contents;
}

}  // namespace test
}  // namespace android
}  // namespace aidl
