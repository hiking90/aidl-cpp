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

#include "logging.h"

using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {
namespace {

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

bool TypeNamespace::AddParcelableType(const AidlParcelable* p,
                                      const string& filename) {
  // TODO Support parcelables b/23600712
  LOG(ERROR) << "Passing parcelables in unimplemented in C++ generation.";
  return true;
}

bool TypeNamespace::AddBinderType(const AidlInterface* b,
                                  const string& filename) {
  // TODO Support passing binders b/24470875
  LOG(ERROR) << "Passing binders is unimplemented in C++ generation.";
  return true;
}

bool TypeNamespace::AddContainerType(const string& type_name) {
  // TODO Support container types b/24470786
  LOG(ERROR) << "Passing container is unimplemented in C++ generation.";
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
