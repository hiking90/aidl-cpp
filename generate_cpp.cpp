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

#include "generate_cpp.h"

#include <memory>
#include <string>

#include "ast_cpp.h"
#include "code_writer.h"
#include "logging.h"

using std::unique_ptr;
using std::string;

namespace android {
namespace aidl {
namespace {

unique_ptr<CppDocument> BuildClientSource(interface_type* parsed_doc) {
  return unique_ptr<CppDocument>{new CppSource{
      {},
      new CppNamespace{
          "android",
          {}
      }
  }};
}

unique_ptr<CppDocument> BuildServerSource(interface_type* parsed_doc) {
  return unique_ptr<CppDocument>{new CppSource{
      {},
      new CppNamespace{
          "android",
          {}
      }
  }};
}

unique_ptr<CppDocument> BuildInterfaceSource(interface_type* parsed_doc) {
  return unique_ptr<CppDocument>{new CppSource{
      {},
      new CppNamespace{
          "android",
          {}
      }
  }};
}

unique_ptr<CppDocument> BuildClientHeader(interface_type* parsed_doc) {
  return unique_ptr<CppDocument>{new CppHeader{
      "FILL_ME_IN",
      {},
      new CppNamespace{
          "android",
          {}
      }
  }};
}

unique_ptr<CppDocument> BuildServerHeader(interface_type* parsed_doc) {
  return unique_ptr<CppDocument>{new CppHeader{
      "FILL_ME_IN",
      {},
      new CppNamespace{
          "android",
          {}
      }
  }};
}

unique_ptr<CppDocument> BuildInterfaceHeader(interface_type* parsed_doc) {
  return unique_ptr<CppDocument>{new CppHeader{
      "FILL_ME_IN",
      {},
      new CppNamespace{
          "android",
          {}
      }
  }};
}

bool GenerateCppForFile(const std::string& name, unique_ptr<CppDocument> doc) {
  if (!doc) {
    return false;
  }
  unique_ptr<CodeWriter> writer = GetFileWriter(name);
  doc->Write(writer.get());
  return true;
}

}  // namespace

bool GenerateCpp(const CppOptions& options, interface_type* parsed_doc) {
  bool success = true;

  success &= GenerateCppForFile(options.ClientCppFileName(),
                                BuildClientSource(parsed_doc));
  success &= GenerateCppForFile(options.ClientHeaderFileName(),
                                BuildClientHeader(parsed_doc));
  success &= GenerateCppForFile(options.ServerCppFileName(),
                                BuildServerSource(parsed_doc));
  success &= GenerateCppForFile(options.ServerHeaderFileName(),
                                BuildServerHeader(parsed_doc));
  success &= GenerateCppForFile(options.InterfaceCppFileName(),
                                BuildInterfaceSource(parsed_doc));
  success &= GenerateCppForFile(options.InterfaceHeaderFileName(),
                                BuildInterfaceHeader(parsed_doc));

  return success;
}

}  // namespace android
}  // namespace aidl
