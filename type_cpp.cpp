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
#include <vector>

#include <base/stringprintf.h>
#include <base/strings.h>

#include "logging.h"

using std::cerr;
using std::endl;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

using android::base::Split;
using android::base::Join;
using android::base::StringPrintf;

namespace android {
namespace aidl {
namespace cpp {
namespace {

const char kNoPackage[] = "";
const char kNoHeader[] = "";
const char kNoValidMethod[] = "";

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
  VoidType() : Type(ValidatableType::KIND_BUILT_IN, kNoPackage, "void",
                    kNoHeader, "void", kNoValidMethod, kNoValidMethod) {}
  virtual ~VoidType() = default;
  bool CanBeOutParameter() const override { return false; }
  bool CanWriteToParcel() const override { return false; }
};  // class VoidType

class BinderType : public Type {
 public:
  BinderType(const AidlInterface& interface, const std::string& src_file_name)
      : Type(ValidatableType::KIND_GENERATED,
             interface.GetPackage(), interface.GetName(),
             GetCppHeader(interface), GetCppName(interface),
             "readStrongBinder", "writeStrongBinder",
             kNoValidMethod, kNoValidMethod,
             src_file_name, interface.GetLine()) {}
  virtual ~BinderType() = default;

  string WriteCast(const string& val) const override {
    return Name() + "::asBinder(" + val + ")";
  }

 private:
  static string GetCppName(const AidlInterface& interface) {
    vector<string> name = interface.GetSplitPackage();
    string ret = "::android::sp<";

    name.push_back(interface.GetName());

    for (const auto& term : name) {
      ret += "::" + term;
    }

    return ret + ">";
  }

  static string GetCppHeader(const AidlInterface& interface) {
    vector<string> name = interface.GetSplitPackage();
    name.push_back(interface.GetName());
    return Join(name, '/') + ".h";
  }
};

class ParcelableType : public Type {
 public:
  ParcelableType(const AidlParcelable& parcelable,
                 const std::string& src_file_name)
      : Type(ValidatableType::KIND_PARCELABLE,
             parcelable.GetPackage(), parcelable.GetName(),
             parcelable.GetCppHeader(), GetCppName(parcelable),
             "readParcelable", "writeParcelable",
             "readParcelableVector", "writeParcelableVector",
             src_file_name, parcelable.GetLine()) {}
  virtual ~ParcelableType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  static string GetCppName(const AidlParcelable& parcelable) {
    return "::" + Join(parcelable.GetSplitPackage(), "::") +
        "::" + parcelable.GetName();
  }
};

class StringListType : public Type {
 public:
  StringListType()
      : Type(ValidatableType::KIND_BUILT_IN, "java.util", "List<String>",
            "utils/String16.h", "::std::vector<::android::String16>",
             "readString16Vector", "writeString16Vector") {}
  virtual ~StringListType() = default;
  bool CanBeOutParameter() const override { return true; }

  void GetHeaders(bool is_array, set<string>* headers) const override {
    if (is_array) {
      LOG(FATAL) << "Type checking did not catch that List<String> "
                    "was marked as array";
    }
    Type::GetHeaders(is_array, headers);
    headers->insert("vector");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(StringListType);
};  // class StringListType

class BinderListType : public Type {
 public:
  BinderListType()
      : Type(ValidatableType::KIND_BUILT_IN, "java.util",
             "List<android.os.IBinder>", "binder/IBinder.h",
             "::std::vector<::android::sp<::android::IBinder>>",
             "readStrongBinderVector", "writeStrongBinderVector") {}
  virtual ~BinderListType() = default;
  bool CanBeOutParameter() const override { return true; }

  void GetHeaders(bool is_array, set<string>* headers) const override {
    if (is_array) {
      LOG(FATAL) << "Type checking did not catch that List<Binder> "
                    "was marked as array";
    }
    Type::GetHeaders(is_array, headers);
    headers->insert("vector");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BinderListType);
};  // class BinderListType

}  // namespace

Type::Type(int kind,
           const std::string& package,
           const std::string& aidl_type,
           const string& header,
           const string& cpp_type,
           const string& read_method,
           const string& write_method,
           const string& read_array_method,
           const string& write_array_method,
           const string& src_file_name,
           int line)
    : ValidatableType(kind, package, aidl_type, src_file_name, line),
      header_(header),
      aidl_type_(aidl_type),
      cpp_type_(cpp_type),
      parcel_read_method_(read_method),
      parcel_write_method_(write_method),
      parcel_read_array_method_(read_array_method),
      parcel_write_array_method_(write_array_method) {}

bool Type::CanBeArray() const { return ! parcel_read_array_method_.empty(); }
bool Type::CanWriteToParcel() const { return true; }
void Type::GetHeaders(bool is_array, set<string>* headers) const {
  if (!header_.empty()) {
    headers->insert(header_);
  }
  if (is_array) {
    headers->insert("vector");
  }
}

string Type::CppType(bool is_array) const {
  if (is_array) {
    return "::std::vector<" + cpp_type_ + ">";
  } else {
    return cpp_type_;
  }
}

const string& Type::ReadFromParcelMethod(bool is_array) const {
  if (is_array) {
    return parcel_read_array_method_;
  } else {
    return parcel_read_method_;
  }
}

const string& Type::WriteToParcelMethod(bool is_array) const {
  if (is_array) {
    return parcel_write_array_method_;
  } else {
    return parcel_write_method_;
  }
}

void TypeNamespace::Init() {
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "byte",
      "cstdint", "int8_t", "readByte", "writeByte",
      "readByteVector", "writeByteVector"));
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "int",
      "cstdint", "int32_t", "readInt32", "writeInt32",
      "readInt32Vector", "writeInt32Vector"));
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "long",
      "cstdint", "int64_t", "readInt64", "writeInt64",
      "readInt64Vector", "writeInt64Vector"));
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "float",
      kNoHeader, "float", "readFloat", "writeFloat",
      "readFloatVector", "writeFloatVector"));
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "double",
      kNoHeader, "double", "readDouble", "writeDouble",
      "readDoubleVector", "writeDoubleVector"));
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "boolean",
      kNoHeader, "bool", "readBool", "writeBool",
      "readBoolVector", "writeBoolVector"));
  // C++11 defines the char16_t type as a built in for Unicode characters.
  Add(new PrimitiveType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "char",
      kNoHeader, "char16_t", "readChar", "writeChar",
      "readCharVector", "writeCharVector"));

  string_type_ = new Type(ValidatableType::KIND_BUILT_IN, kNoPackage, "String",
                          "utils/String16.h", "::android::String16",
                          "readString16", "writeString16",
                          "readString16Vector", "writeString16Vector");
  Add(string_type_);

  ibinder_type_ = new Type(ValidatableType::KIND_BUILT_IN, "android.os",
                           "IBinder", "binder/IBinder.h",
                           "::android::sp<::android::IBinder>", "readStrongBinder",
                           "writeStrongBinder");
  Add(ibinder_type_);

  Add(new BinderListType());
  Add(new StringListType());

  Add(new Type(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "FileDescriptor",
      "nativehelper/ScopedFd.h", "::ScopedFd",
      "readUniqueFileDescriptor", "writeUniqueFileDescriptor",
      "readUniqueFileDescriptorVector", "writeUniqueFileDescriptorVector"));

  void_type_ = new class VoidType();
  Add(void_type_);
}

bool TypeNamespace::AddParcelableType(const AidlParcelable& p,
                                      const string& filename) {
  Add(new ParcelableType(p, filename));
  return true;
}

bool TypeNamespace::AddBinderType(const AidlInterface& b,
                                  const string& file_name) {
  Add(new BinderType(b, file_name));
  return true;
}

bool TypeNamespace::AddListType(const std::string& type_name) {
  const Type* contained_type = Find(type_name);
  if (!contained_type) {
    LOG(ERROR) << "Cannot create List<" << type_name << "> because contained "
                  "type cannot be found or is invalid.";
    return false;
  }
  if (contained_type->IsCppPrimitive()) {
    LOG(ERROR) << "Cannot create List<" << type_name << "> because contained "
                  "type is a primitive in Java and Java List cannot hold "
                  "primitives.";
    return false;
  }

  if (contained_type == StringType() || contained_type == IBinderType()) {
    return true;
  }

  // TODO Support lists of parcelables b/23600712

  LOG(ERROR) << "aidl-cpp does not yet support List<" << type_name << ">";
  return false;
}

bool TypeNamespace::AddMapType(const std::string& /* key_type_name */,
                               const std::string& /* value_type_name */) {
  // TODO Support list types b/25242025
  LOG(ERROR) << "aidl does not implement support for typed maps!";
  return false;
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

}  // namespace cpp
}  // namespace aidl
}  // namespace android
