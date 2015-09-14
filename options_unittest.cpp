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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "options.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace {

const char kPreprocessCommandOutputFile[] = "output_file_name";
const char kPreprocessCommandInput1[] = "input1";
const char kPreprocessCommandInput2[] = "input2";
const char kPreprocessCommandInput3[] = "input3";
const char* kPreprocessCommand[] = {
    "aidl", "--preprocess",
    kPreprocessCommandOutputFile,
    kPreprocessCommandInput1,
    kPreprocessCommandInput2,
    kPreprocessCommandInput3,
};

}  // namespace

TEST(OptionsTests, ParsesPreprocess) {
  const int argc = sizeof(kPreprocessCommand) / sizeof(*kPreprocessCommand);
  unique_ptr<Options> options(Options::ParseOptions(argc, kPreprocessCommand));
  EXPECT_NE(options, nullptr);
  EXPECT_EQ(options->task, Options::PREPROCESS_AIDL);
  EXPECT_EQ(options->fail_on_parcelable_, false);
  EXPECT_EQ(options->import_paths_.size(), 0u);
  EXPECT_EQ(options->preprocessed_files_.size(), 0u);
  EXPECT_EQ(options->input_file_name_, string{});
  EXPECT_EQ(options->output_file_name_, string{kPreprocessCommandOutputFile});
  EXPECT_EQ(options->auto_dep_file_, false);
  const vector<string> expected_input{kPreprocessCommandInput1,
                                      kPreprocessCommandInput2,
                                      kPreprocessCommandInput3};
  EXPECT_EQ(options->files_to_preprocess_, expected_input);
}

}  // namespace android
}  // namespace aidl
