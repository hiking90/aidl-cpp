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

#ifndef AIDL_AST_JAVA_H_
#define AIDL_AST_JAVA_H_

#include <string>
#include <vector>

#include <base/macros.h>

namespace android {
namespace aidl {

class CodeWriter;

class CppHeader final {
 public:
  CppHeader(const std::string& include_guard,
            const std::vector<std::string>& include_list,
            const std::vector<std::string>& namespaces);
  virtual void Write(CodeWriter* to) const;

 private:
  const std::string include_guard_;
  std::vector<std::string> include_list_;
  std::vector<std::string> namespaces_;

  DISALLOW_COPY_AND_ASSIGN(CppHeader);
};  // class CppHeader

class CppDocument final {
 public:
  CppDocument(const std::vector<std::string>& include_list,
              const std::vector<std::string>& namespaces);
  virtual void Write(CodeWriter* to) const;

 private:
  std::vector<std::string> include_list_;
  std::vector<std::string> namespaces_;

  DISALLOW_COPY_AND_ASSIGN(CppDocument);
};  // class CppDocument

}  // namespace aidl
}  // namespace android

#endif // AIDL_AST_JAVA_H_
