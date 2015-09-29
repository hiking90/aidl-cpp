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

#ifndef AIDL_TYPE_NAMESPACE_H_
#define AIDL_TYPE_NAMESPACE_H_

#include <memory>
#include <string>

#include <base/macros.h>

#include "aidl_language.h"

namespace android {
namespace aidl {

class ValidatableType {
 public:
  ValidatableType() = default;
  virtual ~ValidatableType() = default;

  virtual bool CanBeArray() const = 0;
  virtual bool CanBeOutParameter() const = 0;
  virtual bool CanWriteToParcel() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ValidatableType);
};

class TypeNamespace {
 public:
  // Load this TypeNamespace with user defined types.
  virtual bool AddParcelableType(const user_data_type* p,
                                 const std::string& filename) = 0;
  virtual bool AddBinderType(const interface_type* b,
                             const std::string& filename) = 0;
  // We dynamically create container types as we discover them in the parse
  // tree.  Returns false iff this is an invalid type.  Silently discards
  // duplicates and non-container types.
  virtual bool AddContainerType(const std::string& type_name) = 0;

  // Returns true iff this has a type for |type_name|.
  virtual bool HasType(const std::string& type_name) const;

  // Returns true iff |raw_type| is a valid return type.
  virtual bool IsValidReturnType(const type_type* raw_type,
                                 const std::string& filename) const;

  // Returns true iff |arg_type| is a valid method argument.
  virtual bool IsValidArg(const AidlArgument& a,
                          int arg_index,
                          const std::string& filename) const;

 protected:
  TypeNamespace() = default;
  virtual ~TypeNamespace() = default;

  virtual const ValidatableType* GetValidatableType(
      const std::string& name) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TypeNamespace);
};

}  // namespace aidl
}  // namespace android

#endif  // AIDL_TYPE_NAMESPACE_H_
