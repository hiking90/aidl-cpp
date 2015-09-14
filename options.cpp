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

#include "options.h"

#include <stdio.h>

using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace {

unique_ptr<Options> usage() {
  fprintf(stderr,
          "usage: aidl OPTIONS INPUT [OUTPUT]\n"
          "       aidl --preprocess OUTPUT INPUT...\n"
          "\n"
          "OPTIONS:\n"
          "   -I<DIR>    search path for import statements.\n"
          "   -d<FILE>   generate dependency file.\n"
          "   -a         generate dependency file next to the output file with "
          "the name based on the input file.\n"
          "   -p<FILE>   file created by --preprocess to import.\n"
          "   -o<FOLDER> base output folder for generated files.\n"
          "   -b         fail when trying to compile a parcelable.\n"
          "\n"
          "INPUT:\n"
          "   An aidl interface file.\n"
          "\n"
          "OUTPUT:\n"
          "   The generated interface files.\n"
          "   If omitted and the -o option is not used, the input filename is "
          "used, with the .aidl extension changed to a .java extension.\n"
          "   If the -o option is used, the generated files will be placed in "
          "the base output folder, under their package folder\n");
  return unique_ptr<Options>(nullptr);
}

}  // namespace

unique_ptr<Options> Options::ParseOptions(int argc, const char* const* argv) {
  unique_ptr<Options> options(new Options());
  int i = 1;

  if (argc >= 2 && 0 == strcmp(argv[1], "--preprocess")) {
    if (argc < 4) {
      return usage();
    }
    options->output_file_name_ = argv[2];
    for (int i = 3; i < argc; i++) {
      options->files_to_preprocess_.push_back(argv[i]);
    }
    options->task = PREPROCESS_AIDL;
    return options;
  }

  options->task = COMPILE_AIDL_TO_JAVA;
  // OPTIONS
  while (i < argc) {
    const char* s = argv[i];
    const size_t len = strlen(s);
    if (s[0] != '-') {
      break;
    }
    if (len <= 1) {
      fprintf(stderr, "unknown option (%d): %s\n", i, s);
      return usage();
    }
    // -I<system-import-path>
    if (s[1] == 'I') {
      if (len > 2) {
        options->import_paths_.push_back(s + 2);
      } else {
        fprintf(stderr, "-I option (%d) requires a path.\n", i);
        return usage();
      }
    } else if (s[1] == 'd') {
      if (len > 2) {
        options->dep_file_name_ = s + 2;
      } else {
        fprintf(stderr, "-d option (%d) requires a file.\n", i);
        return usage();
      }
    } else if (strcmp(s, "-a") == 0) {
      options->auto_dep_file_ = true;
    } else if (s[1] == 'p') {
      if (len > 2) {
        options->preprocessed_files_.push_back(s + 2);
      } else {
        fprintf(stderr, "-p option (%d) requires a file.\n", i);
        return usage();
      }
    } else if (s[1] == 'o') {
      if (len > 2) {
        options->output_base_folder_= s + 2;
      } else {
        fprintf(stderr, "-o option (%d) requires a path.\n", i);
        return usage();
      }
    } else if (strcmp(s, "-b") == 0) {
      options->fail_on_parcelable_ = true;
    } else {
      // s[1] is not known
      fprintf(stderr, "unknown option (%d): %s\n", i, s);
      return usage();
    }
    i++;
  }

  // INPUT
  if (i < argc) {
    options->input_file_name_ = argv[i];
    i++;
  } else {
    fprintf(stderr, "INPUT required\n");
    return usage();
  }

  // OUTPUT
  if (i < argc) {
    options->output_file_name_ = argv[i];
    i++;
  } else if (options->output_base_folder_.empty()) {
    // copy input into output and change the extension from .aidl to .java
    options->output_file_name_= options->input_file_name_;
    const size_t suffix_len = 5;  // 5 = strlen(".aidl")
    string::size_type pos = options->output_file_name_.size() - suffix_len;
    if (options->output_file_name_.compare(pos, suffix_len, ".aidl") == 0) {
      options->output_file_name_.replace(pos, suffix_len, ".java");
    } else {
      fprintf(stderr, "INPUT is not an .aidl file.\n");
      return usage();
    }
  }

  // anything remaining?
  if (i != argc) {
    fprintf(stderr, "unknown option%s:",
            (i == argc - 1 ? (const char*)"" : (const char*)"s"));
    for (; i < argc - 1; i++) {
      fprintf(stderr, " %s", argv[i]);
    }
    fprintf(stderr, "\n");
    return usage();
  }

  return options;
}

}  // namespace android
}  // namespace aidl
