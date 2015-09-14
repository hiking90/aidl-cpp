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

#ifndef AIDL_OPTIONS_H_
#define AIDL_OPTIONS_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>

namespace android {
namespace aidl {

// This struct is the parsed version of the command line options
class Options {
 public:
  enum {
      COMPILE_AIDL_TO_JAVA,
      PREPROCESS_AIDL,
  };

  ~Options() = default;

  // Takes the inputs from the command line and returns a pointer to an
  // Options object on success.
  // Also prints the usage statement on failure.
  static std::unique_ptr<Options> ParseOptions(int argc,
                                               const char* const* argv);

  int task{COMPILE_AIDL_TO_JAVA};
  bool fail_on_parcelable_{false};
  std::vector<std::string> import_paths_;
  std::vector<std::string> preprocessed_files_;
  std::string input_file_name_;
  std::string output_file_name_;
  std::string output_base_folder_;
  std::string dep_file_name_;
  bool auto_dep_file_{false};
  std::vector<std::string> files_to_preprocess_;

 private:
  Options() = default;

  FRIEND_TEST(EndToEndTest, IExampleInterface);
  DISALLOW_COPY_AND_ASSIGN(Options);
};

}  // namespace android
}  // namespace aidl

#endif // AIDL_OPTIONS_H_
