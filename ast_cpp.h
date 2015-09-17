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

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

namespace android {
namespace aidl {

class CodeWriter;

class CppNode {
 public:
  CppNode() = default;
  virtual ~CppNode() = default;
  virtual void Write(CodeWriter* to) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CppNode);
};  // class CppNode

class CppDeclaration : public CppNode {
 public:
  CppDeclaration() = default;
  virtual ~CppDeclaration() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(CppDeclaration);
};  // class CppDeclaration

class CppClassDeclaration : public CppDeclaration {
 public:
  CppClassDeclaration(const std::string& name,
                      const std::string& parent,
                      std::vector<CppDeclaration*> public_members,
                      std::vector<CppDeclaration*> private_members);
  virtual ~CppClassDeclaration();

  void Write(CodeWriter* to) const override;

 private:
  std::string name_;
  std::string parent_;
  std::vector<CppDeclaration*> public_members_;
  std::vector<CppDeclaration*> private_members_;

  DISALLOW_COPY_AND_ASSIGN(CppClassDeclaration);
};  // class CppClassDeclaration

class CppMethodDeclaration : public CppDeclaration {
 public:
  CppMethodDeclaration(const std::string& return_type,
                       const std::string& name,
                       std::vector<std::string> arguments,
                       bool is_const = false,
                       bool is_virtual = false);

  virtual ~CppMethodDeclaration() = default;

  void Write(CodeWriter* to) const override;

 private:
  const std::string return_type_;
  const std::string name_;
  std::vector<std::string> arguments_;
  bool is_const_;
  bool is_virtual_;

  DISALLOW_COPY_AND_ASSIGN(CppMethodDeclaration);
};

class CppNamespace : public CppDeclaration {
 public:
  CppNamespace(const std::string& name,
               std::vector<CppDeclaration*> declarations);
  virtual ~CppNamespace();

  void Write(CodeWriter* to) const override;

 private:
  std::vector<CppDeclaration*> declarations_;
  std::string name_;

  DISALLOW_COPY_AND_ASSIGN(CppNamespace);
};  // class CppNamespace

class CppDocument : public CppNode {
 public:
  CppDocument(const std::vector<std::string>& include_list,
              CppNamespace* a_namespace);

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::string> include_list_;
  std::unique_ptr<CppNamespace> namespace_;

  DISALLOW_COPY_AND_ASSIGN(CppDocument);
};  // class CppDocument

class CppHeader final : public CppDocument {
 public:
  CppHeader(const std::string& include_guard,
            const std::vector<std::string>& include_list,
            CppNamespace* a_namespace);
  void Write(CodeWriter* to) const override;

 private:
  const std::string include_guard_;

  DISALLOW_COPY_AND_ASSIGN(CppHeader);
};  // class CppHeader

class CppSource final : public CppDocument {
 public:
  CppSource(const std::vector<std::string>& include_list,
            CppNamespace* a_namespace);

 private:
  DISALLOW_COPY_AND_ASSIGN(CppSource);
};  // class CppSource

}  // namespace aidl
}  // namespace android

#endif // AIDL_AST_JAVA_H_
