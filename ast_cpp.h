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

#ifndef AIDL_AST_CPP_H_
#define AIDL_AST_CPP_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

namespace android {
namespace aidl {
class CodeWriter;
}  // namespace aidl
}  // namespace android

namespace android {
namespace aidl {
namespace cpp {

class AstNode {
 public:
  AstNode() = default;
  virtual ~AstNode() = default;

  virtual void Write(CodeWriter* to) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AstNode);
};  // class AstNode

class Declaration : public AstNode {
 public:
  Declaration() = default;
  virtual ~Declaration() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(Declaration);
};  // class Declaration

class ClassDecl : public Declaration {
 public:
  ClassDecl(const std::string& name,
            const std::string& parent);
  ClassDecl(const std::string& name,
            const std::string& parent,
            std::vector<std::unique_ptr<Declaration>> public_members,
            std::vector<std::unique_ptr<Declaration>> private_members);
  virtual ~ClassDecl() = default;

  void Write(CodeWriter* to) const override;

  void AddPublic(std::unique_ptr<Declaration> member);
  void AddPrivate(std::unique_ptr<Declaration> member);

 private:
  std::string name_;
  std::string parent_;
  std::vector<std::unique_ptr<Declaration>> public_members_;
  std::vector<std::unique_ptr<Declaration>> private_members_;

  DISALLOW_COPY_AND_ASSIGN(ClassDecl);
};  // class ClassDecl

class Enum : public Declaration {
 public:
  explicit Enum(const std::string& name);
  virtual ~Enum() = default;

  void Write(CodeWriter* to) const override;

  void AddValue(const std::string& key, const std::string& value);

 private:
  struct EnumField {
    EnumField(const std::string& k, const std::string& v);
    const std::string key;
    const std::string value;
  };

  std::string enum_name_;
  std::vector<EnumField> fields_;

  DISALLOW_COPY_AND_ASSIGN(Enum);
};  // class Enum

class ArgList : public AstNode {
 public:
  explicit ArgList(const std::string& single_argument);
  explicit ArgList(const std::vector<std::string>& arg_list);
  virtual ~ArgList() = default;

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::string> arguments_;

  DISALLOW_COPY_AND_ASSIGN(ArgList);
};  // class ArgList

class ConstructorDecl : public Declaration {
 public:
  ConstructorDecl(const std::string& name,
                  std::vector<std::string> arguments);
  ConstructorDecl(const std::string& name,
                  std::vector<std::string> arguments,
                  bool is_virtual,
                  bool is_default);

  virtual ~ConstructorDecl() = default;

  void Write(CodeWriter* to) const override;

 private:
  const std::string name_;
  std::vector<std::string> arguments_;
  bool is_virtual_ = false;
  bool is_default_ = false;

  DISALLOW_COPY_AND_ASSIGN(ConstructorDecl);
};  // class ConstructorDecl

class MethodDecl : public Declaration {
 public:
  enum Modifiers {
    IS_CONST = 1 << 0,
    IS_VIRTUAL = 1 << 1,
    IS_OVERRIDE = 1 << 2,
    IS_PURE_VIRTUAL = 1 << 3,
  };

  MethodDecl(const std::string& return_type,
             const std::string& name,
             std::vector<std::string> arguments);
  MethodDecl(const std::string& return_type,
             const std::string& name,
             std::vector<std::string> arguments,
             uint32_t modifiers);
  virtual ~MethodDecl() = default;

  void Write(CodeWriter* to) const override;

 private:
  const std::string return_type_;
  const std::string name_;
  std::vector<std::string> arguments_;
  bool is_const_ = false;
  bool is_virtual_ = false;
  bool is_override_ = false;
  bool is_pure_virtual_ = false;

  DISALLOW_COPY_AND_ASSIGN(MethodDecl);
};  // class MethodDecl

class StatementBlock : public Declaration {
 public:
  StatementBlock() = default;
  virtual ~StatementBlock() = default;

  void AddStatement(std::unique_ptr<AstNode> statement);
  void AddStatement(AstNode* statement);  // Takes ownership
  void AddLiteral(const std::string& expression, bool add_semicolon = true);

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::unique_ptr<AstNode>> statements_;

  DISALLOW_COPY_AND_ASSIGN(StatementBlock);
};  // class StatementBlock

class MethodImpl : public Declaration {
 public:
  // Passing an empty class name causes the method to be declared as a normal
  // function (ie. no ClassName:: qualifier).
  MethodImpl(const std::string& return_type,
             const std::string& class_name,
             const std::string& method_name,
             std::vector<std::string> arguments,
             bool is_const_method = false);
  virtual ~MethodImpl() = default;

  void AddStatement(std::unique_ptr<AstNode> statement);
  void Write(CodeWriter* to) const override;

 private:
  std::string return_type_;
  std::string method_name_;
  std::vector<std::string> arguments_;
  StatementBlock statements_;
  bool is_const_method_ = false;

  DISALLOW_COPY_AND_ASSIGN(MethodImpl);
};  // class MethodImpl

class SwitchStatement : public AstNode {
 public:
  explicit SwitchStatement(const std::string& expression);
  virtual ~SwitchStatement() = default;

  // Add a case statement and return a pointer code block corresponding
  // to the case.  The switch statement will add a break statement
  // after the code block by default to prevent accidental fall-through.
  // Returns nullptr on duplicate value expressions (by strcmp, not value
  // equivalence).
  StatementBlock* AddCase(const std::string& value_expression);
  void Write(CodeWriter* to) const override;

 private:
  const std::string switch_expression_;
  std::vector<std::string> case_values_;
  std::vector<std::unique_ptr<StatementBlock>> case_logic_;

  DISALLOW_COPY_AND_ASSIGN(SwitchStatement);
};  // class SwitchStatement

class Assignment : public AstNode {
 public:
  Assignment(const std::string& left, const std::string& right);
  Assignment(const std::string& left, AstNode* right);
  ~Assignment() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string lhs_;
  std::unique_ptr<AstNode> rhs_;

  DISALLOW_COPY_AND_ASSIGN(Assignment);
};  // class Assignment

class MethodCall : public AstNode {
 public:
  MethodCall(const std::string& method_name,
             const std::string& single_argument);
  MethodCall(const std::string& method_name, ArgList* arg_list);
  ~MethodCall() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string method_name_;
  const std::unique_ptr<ArgList> arg_list_;

  DISALLOW_COPY_AND_ASSIGN(MethodCall);
};  // class MethodCall

class LiteralStatement : public AstNode {
 public:
  LiteralStatement(const std::string& expression, bool use_semicolon = true);
  ~LiteralStatement() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string expression_;
  bool use_semicolon_;

  DISALLOW_COPY_AND_ASSIGN(LiteralStatement);
};  // class LiteralStatement

class LiteralExpression : public AstNode {
 public:
  explicit LiteralExpression(const std::string& expression);
  ~LiteralExpression() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string expression_;

  DISALLOW_COPY_AND_ASSIGN(LiteralExpression);
};  // class LiteralExpression

class CppNamespace : public Declaration {
 public:
  CppNamespace(const std::string& name,
               std::vector<std::unique_ptr<Declaration>> declarations);
  CppNamespace(const std::string& name,
               std::unique_ptr<Declaration> declaration);
  CppNamespace(const std::string& name);
  virtual ~CppNamespace() = default;

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::unique_ptr<Declaration>> declarations_;
  std::string name_;

  DISALLOW_COPY_AND_ASSIGN(CppNamespace);
};  // class CppNamespace

class Document : public AstNode {
 public:
  Document(const std::vector<std::string>& include_list,
           std::unique_ptr<CppNamespace> a_namespace);

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::string> include_list_;
  std::unique_ptr<CppNamespace> namespace_;

  DISALLOW_COPY_AND_ASSIGN(Document);
};  // class Document

class CppHeader final : public Document {
 public:
  CppHeader(const std::string& include_guard,
            const std::vector<std::string>& include_list,
            std::unique_ptr<CppNamespace> a_namespace);
  void Write(CodeWriter* to) const override;

 private:
  const std::string include_guard_;

  DISALLOW_COPY_AND_ASSIGN(CppHeader);
};  // class CppHeader

class CppSource final : public Document {
 public:
  CppSource(const std::vector<std::string>& include_list,
            std::unique_ptr<CppNamespace> a_namespace);

 private:
  DISALLOW_COPY_AND_ASSIGN(CppSource);
};  // class CppSource

}  // namespace cpp
}  // namespace aidl
}  // namespace android

#endif // AIDL_AST_CPP_H_
