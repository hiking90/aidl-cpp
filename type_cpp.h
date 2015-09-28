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

#ifndef AIDL_TYPE_CPP_H_
#define AIDL_TYPE_CPP_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

#include "type_namespace.h"

namespace android {
namespace aidl {
namespace cpp {

class Type : public ValidatableType {
 public:
  Type(const std::string& aidl_type,
       const std::string& cpp_type,
       const std::string& read_method,
       const std::string& write_method);
  virtual ~Type() = default;

  // overrides of ValidatableType
  bool CanBeArray() const override;
  bool CanBeOutParameter() const override;
  bool CanWriteToParcel() const override;

  const std::string& AidlType() const;
  const std::string& CppType() const;
  const std::string& ReadFromParcelMethod() const;
  const std::string& WriteToParcelMethod() const;

 private:
  // |aidl_type| is what we find in the yacc generated AST (e.g. "int").
  const std::string aidl_type_;
  // |cpp_type| is what we use in the generated C++ code (e.g. "int32_t").
  const std::string cpp_type_;
  const std::string parcel_read_method_;
  const std::string parcel_write_method_;

  DISALLOW_COPY_AND_ASSIGN(Type);
};  // class Type


class TypeNamespace : public ::android::aidl::TypeNamespace {
 public:
  TypeNamespace();
  virtual ~TypeNamespace() = default;

  bool AddParcelableType(const user_data_type* p,
                         const std::string& filename) override;
  bool AddBinderType(const interface_type* b,
                     const std::string& filename) override;
  bool AddContainerType(const std::string& type_name) override;

  const Type* Find(const std::string& type_name) const;

 protected:
  const ValidatableType* GetValidatableType(
      const std::string& type_name) const override;

 private:
  std::vector<std::unique_ptr<Type>> types_;

  DISALLOW_COPY_AND_ASSIGN(TypeNamespace);
};  // class TypeNamespace

}  // namespace cpp
}  // namespace aidl
}  // namespace android

#endif  // AIDL_TYPE_NAMESPACE_CPP_H_
