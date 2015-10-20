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

#include "type_cpp.h"

#include <algorithm>
#include <iostream>

#include <base/stringprintf.h>
#include <base/strings.h>

#include "logging.h"

using std::cerr;
using std::endl;
using std::string;
using std::unique_ptr;

using android::base::Split;
using android::base::StringPrintf;

namespace android {
namespace aidl {
namespace cpp {
namespace {

bool is_cpp_keyword(const std::string& str) {
  static const std::vector<std::string> kCppKeywords{
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
    "compl", "concept", "const", "constexpr", "const_cast", "continue",
    "decltype", "default", "delete", "do", "double", "dynamic_cast", "else",
    "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
    "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
    "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
    "private", "protected", "public", "register", "reinterpret_cast",
    "requires", "return", "short", "signed", "sizeof", "static",
    "static_assert", "static_cast", "struct", "switch", "template", "this",
    "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
    "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
    "while", "xor", "xor_eq",
  };
  return std::find(kCppKeywords.begin(), kCppKeywords.end(), str) !=
      kCppKeywords.end();
}

class VoidType : public Type {
 public:
  VoidType() : Type("", "void", "void", "XXX", "XXX") {}
  virtual ~VoidType() = default;
  bool CanBeArray() const override { return false; }
  bool CanBeOutParameter() const override { return false; }
  bool CanWriteToParcel() const override { return false; }
};  // class VoidType

}  // namespace

Type::Type(const string& header,
           const string& aidl_type,
           const string& cpp_type,
           const string& read_method,
           const string& write_method)
    : header_(header),
      aidl_type_(aidl_type),
      cpp_type_(cpp_type),
      parcel_read_method_(read_method),
      parcel_write_method_(write_method) {
}

bool Type::CanBeArray() const { return false; }
bool Type::CanBeOutParameter() const { return false; }
bool Type::CanWriteToParcel() const { return true; }
const string& Type::AidlType() const { return aidl_type_; }
const string& Type::Header() const { return header_; }
const string& Type::CppType() const { return cpp_type_; }
const string& Type::ReadFromParcelMethod() const { return parcel_read_method_; }
const string& Type::WriteToParcelMethod() const { return parcel_write_method_; }

TypeNamespace::TypeNamespace() {
  types_.emplace_back(
      new Type("cstdint", "byte", "int8_t", "readByte", "writeByte"));

  types_.emplace_back(
      new Type("cstdint", "int", "int32_t", "readInt32", "writeInt32"));
  types_.emplace_back(
      new Type("cstdint", "long", "int64_t", "readInt64", "writeInt64"));
  types_.emplace_back(
      new Type("", "float", "float", "readFloat", "writeFloat"));
  types_.emplace_back(
      new Type("", "double", "double", "readDouble", "writeDouble"));
  types_.emplace_back(
      new Type("", "boolean", "bool", "readBool", "writeBool"));
  // For whatever reason, char16_t does not need cstdint.
  types_.emplace_back(
      new Type("", "char", "char16_t", "readChar", "writeChar"));
  types_.emplace_back(
      new Type("utils/String16.h", "String", "android::String16",
               "readString16", "writeString16"));

  void_type_ = new class VoidType();
  types_.emplace_back(void_type_);
}

bool TypeNamespace::AddParcelableType(const AidlParcelable* /* p */,
                                      const string& /* filename */) {
  // TODO Support parcelables b/23600712
  LOG(ERROR) << "Passing parcelables in unimplemented in C++ generation.";
  return true;
}

bool TypeNamespace::AddBinderType(const AidlInterface* /* b */,
                                  const string& /* filename */) {
  // TODO Support passing binders b/24470875
  LOG(ERROR) << "Passing binders is unimplemented in C++ generation.";
  return true;
}

bool TypeNamespace::AddContainerType(const string& /* type_name */) {
  // TODO Support container types b/24470786
  LOG(ERROR) << "Passing container is unimplemented in C++ generation.";
  return true;
}

bool TypeNamespace::IsValidPackage(const string& package) const {
  if (package.empty()) {
    return false;
  }

  auto pieces = Split(package, ".");
  for (const string& piece : pieces) {
    if (is_cpp_keyword(piece)) {
      return false;
    }
  }

  return true;
}

bool TypeNamespace::IsValidArg(const AidlArgument& a,
                               int arg_index,
                               const std::string& filename) const {
  if (!::android::aidl::TypeNamespace::IsValidArg(a, arg_index, filename)) {
    return false;
  }
  const string error_prefix = StringPrintf(
      "In file %s line %d parameter %s (%d):\n    ",
      filename.c_str(), a.GetLine(), a.GetName().c_str(), arg_index);

  // check that the name doesn't match a keyword
  if (is_cpp_keyword(a.GetName().c_str())) {
    cerr << error_prefix << "Argument name is a C++ keyword"
         << endl;
    return false;
  }

  return true;
}

const Type* TypeNamespace::Find(const string& type_name) const {
  Type* ret = nullptr;
  for (const unique_ptr<Type>& type : types_) {
    if (type->AidlType() == type_name) {
      ret = type.get();
      break;
    }
  }

  return ret;
}

const ValidatableType* TypeNamespace::GetValidatableType(
    const string& type_name) const {
  return Find(type_name);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
