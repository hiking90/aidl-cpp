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

#include "code_writer.h"

#include <stdarg.h>

#include <base/stringprintf.h>

namespace android {
namespace aidl {

namespace {

class StringCodeWriter : public CodeWriter {
 public:
  StringCodeWriter(std::string* output_buffer) : output_(output_buffer) {}

  bool Write(const char* format, ...) override {
    va_list ap;
    va_start(ap, format);
    android::base::StringAppendV(output_, format, ap);
    va_end(ap);
    return true;
  }

 private:
  std::string* output_;
};  // class StringCodeWriter

class FileCodeWriter : public CodeWriter {
 public:
  FileCodeWriter(FILE* output_file) : output_(output_file) {}
  ~FileCodeWriter() {
    fclose(output_);
  }

  bool Write(const char* format, ...) override {
    bool success;
    va_list ap;
    va_start(ap, format);
    success = vfprintf(output_, format, ap) >= 0;
    va_end(ap);
    return success;
  }

 private:
  FILE* output_;
};  // class StringCodeWriter

}  // namespace

CodeWriterPtr get_file_writer(FILE* output_file) {
  return CodeWriterPtr(new FileCodeWriter(output_file));
}

CodeWriterPtr get_string_writer(std::string* output_buffer) {
  return CodeWriterPtr(new StringCodeWriter(output_buffer));
}

}  // namespace aidl
}  // namespace android
