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

#include "io_delegate.h"

#include <fstream>

using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {

unique_ptr<string> IoDelegate::GetFileContents(
    const string& filename,
    const string& content_suffix) const {
  unique_ptr<string> contents;
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in) {
    return contents;
  }
  contents.reset(new string);
  in.seekg(0, std::ios::end);
  ssize_t file_size = in.tellg();
  contents->resize(file_size + content_suffix.length());
  in.seekg(0, std::ios::beg);
  // Read the file contents into the beginning of the string
  in.read(&(*contents)[0], file_size);
  // Drop the suffix in at the end.
  contents->replace(file_size, content_suffix.length(), content_suffix);
  in.close();

  return contents;
}

bool IoDelegate::FileIsReadable(const string& path) const {
#ifdef _WIN32
  // check that the file exists and is not write-only
  return (0 == _access(path.c_str(), 0)) &&  // mode 0=exist
         (0 == _access(path.c_str(), 4));    // mode 4=readable
#else
  return (0 == access(path.c_str(), R_OK));
#endif
}
}  // namespace android
}  // namespace aidl
