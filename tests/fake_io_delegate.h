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

#ifndef AIDL_TESTS_FAKE_IO_DELEGATE_H_
#define AIDL_TESTS_FAKE_IO_DELEGATE_H_

#include <base/macros.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "io_delegate.h"

namespace android {
namespace aidl {
namespace test {

class FakeIoDelegate : public IoDelegate {
 public:
  FakeIoDelegate() = default;
  virtual ~FakeIoDelegate() = default;

  // Returns a unique_ptr to the contents of |filename|.
  std::unique_ptr<std::string> GetFileContents(
      const std::string& filename,
      const std::string& append_content_suffix = "") const override;

  bool FileIsReadable(const std::string& path) const override;

  void SetFileContents(const std::string& filename,
                       const std::string& contents);
  void AddStubParcelable(const std::string& canonical_name);
  void AddStubInterface(const std::string& canonical_name);
  void AddCompoundParcelable(const std::string& canonical_name,
                             const std::vector<std::string>& subclasses);

 private:
  void AddStub(const std::string& canonical_name, const char* format_str);
  // Remove leading "./" from |path|.
  std::string CleanPath(const std::string& path) const;

  std::map<std::string, std::string> file_contents_;

  DISALLOW_COPY_AND_ASSIGN(FakeIoDelegate);
};  // class FakeIoDelegate

}  // namespace test
}  // namespace android
}  // namespace aidl

#endif // AIDL_TESTS_FAKE_IO_DELEGATE_H_
