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

#include <memory>
#include <string>
#include <vector>

#include <base/logging.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/stringprintf.h>
#include <gtest/gtest.h>

#include "aidl.h"
#include "options.h"
#include "tests/fake_io_delegate.h"
#include "tests/test_data.h"
#include "tests/test_util.h"

using android::aidl::test::CanonicalNameToPath;
using android::aidl::test::FakeIoDelegate;
using android::base::StringAppendF;
using android::base::StringPrintf;
using base::CreateDirectory;
using base::CreateNewTempDirectory;
using base::FilePath;
using base::WriteFile;
using std::string;
using std::unique_ptr;
using std::vector;

using namespace aidl::test_data;

namespace android {
namespace aidl {
namespace {

const char kDiffTemplate[] = "diff -u %s %s";

}  // namespace

class EndToEndTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    ASSERT_TRUE(CreateNewTempDirectory(
        string{"end_to_end_testsyyyy"}, &tmpDir_));
    outputDir_ = tmpDir_.Append("output");
    ASSERT_TRUE(CreateDirectory(outputDir_));
  }

  virtual void TearDown() {
    ASSERT_TRUE(DeleteFile(tmpDir_, true))
        << "Failed to remove temp directory: " << tmpDir_.value();
  }

  void AddStubAidls(const char** parcelables, const char** interfaces,
                    FakeIoDelegate* io_delegate) {
    for ( ; *parcelables; ++parcelables) {
      io_delegate->AddStubParcelable(*parcelables);
    }
    for ( ; *interfaces; ++interfaces) {
      io_delegate->AddStubInterface(*interfaces);
    }
  }

  void CheckFileContents(const FilePath& rel_path,
                         const string& expected_content) {
    string actual_contents;
    FilePath actual_path = outputDir_.Append(rel_path);
    if (!ReadFileToString(actual_path, &actual_contents)) {
      FAIL() << "Failed to read expected output file: " << rel_path.value();
    }

    if (actual_contents != expected_content) {
      // When the match fails, display a diff of what's wrong.  This greatly
      // aids in debugging.
      FilePath expected_path;
      EXPECT_TRUE(CreateTemporaryFileInDir(tmpDir_, &expected_path));
      WriteFile(expected_path, expected_content.c_str(),
                expected_content.length());
      const size_t buf_len =
          strlen(kDiffTemplate) + actual_path.value().length() +
          expected_path.value().length() + 1;
      unique_ptr<char[]> diff_cmd(new char[buf_len]);
      EXPECT_GT(snprintf(diff_cmd.get(), buf_len, kDiffTemplate,
                         expected_path.value().c_str(),
                         actual_path.value().c_str()), 0);
      system(diff_cmd.get());
      FAIL() << "Actual contents of " << rel_path.value()
             << " did not match expected content";
    }
  }

  FilePath tmpDir_;
  FilePath outputDir_;
};

TEST_F(EndToEndTest, IExampleInterface) {
  FakeIoDelegate io_delegate;
  JavaOptions options;
  options.fail_on_parcelable_ = true;
  options.import_paths_.push_back("");
  options.input_file_name_ =
      CanonicalNameToPath(kIExampleInterfaceClass, ".aidl").value();
  options.output_file_name_for_deps_test_ =
      CanonicalNameToPath(kIExampleInterfaceClass, ".java").value();
  options.output_base_folder_ = outputDir_.value();
  options.dep_file_name_ = outputDir_.Append(FilePath("test.d")).value();

  // Load up our fake file system with data.
  io_delegate.SetFileContents(options.input_file_name_,
                              kIExampleInterfaceContents);
  io_delegate.AddCompoundParcelable("android.test.CompoundParcelable", 
                                    {"Subclass1", "Subclass2"});
  AddStubAidls(kIExampleInterfaceParcelables, kIExampleInterfaceInterfaces,
               &io_delegate);

  // Check that we parse correctly.
  EXPECT_EQ(android::aidl::compile_aidl_to_java(options, io_delegate), 0);
  CheckFileContents(CanonicalNameToPath(kIExampleInterfaceClass, ".java"),
                    kIExampleInterfaceJava);
  CheckFileContents(FilePath("test.d"), kIExampleInterfaceDeps);
}

}  // namespace android
}  // namespace aidl
