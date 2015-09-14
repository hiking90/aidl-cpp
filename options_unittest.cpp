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
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "options.h"

using std::cerr;
using std::endl;
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
    nullptr,
};

const char kCompileCommandInput[] = "input.aidl";
const char kCompileCommandIncludePath[] = "-Iinclude_path";
const char* kCompileJavaCommand[] = {
    "aidl",
    "-b",
    kCompileCommandIncludePath,
    kCompileCommandInput,
    nullptr,
};
const char kCompileCommandOutput[] = "input.java";

unique_ptr<Options> GetOptions(const char* command[]) {
  int argc = 0;
  const char** command_part = command;
  for (; *command_part; ++argc, ++command_part) {}
  unique_ptr<Options> options(Options::ParseOptions(argc, command));
  if (!options) {
    cerr << "Failed to parse command line:";
    for (int i = 0; i < argc; ++i) {
      cerr << " " << command[i];
      cerr << endl;
    }
  }
  EXPECT_NE(options, nullptr) << "Failed to parse options!";
  return options;
}

}  // namespace

TEST(OptionsTests, ParsesPreprocess) {
  unique_ptr<Options> options = GetOptions(kPreprocessCommand);
  EXPECT_EQ(Options::PREPROCESS_AIDL, options->task);
  EXPECT_EQ(false, options->fail_on_parcelable_);
  EXPECT_EQ(0u, options->import_paths_.size());
  EXPECT_EQ(0u, options->preprocessed_files_.size());
  EXPECT_EQ(string{}, options->input_file_name_);
  EXPECT_EQ(string{kPreprocessCommandOutputFile}, options->output_file_name_);
  EXPECT_EQ(false, options->auto_dep_file_);
  const vector<string> expected_input{kPreprocessCommandInput1,
                                      kPreprocessCommandInput2,
                                      kPreprocessCommandInput3};
  EXPECT_EQ(expected_input, options->files_to_preprocess_);
}

TEST(OptionsTests, ParsesCompileJava) {
  unique_ptr<Options> options = GetOptions(kCompileJavaCommand);
  EXPECT_EQ(Options::COMPILE_AIDL_TO_JAVA, options->task);
  EXPECT_EQ(true, options->fail_on_parcelable_);
  EXPECT_EQ(1u, options->import_paths_.size());
  EXPECT_EQ(0u, options->preprocessed_files_.size());
  EXPECT_EQ(string{kCompileCommandInput}, options->input_file_name_);
  EXPECT_EQ(string{kCompileCommandOutput}, options->output_file_name_);
  EXPECT_EQ(false, options->auto_dep_file_);
}

}  // namespace android
}  // namespace aidl
