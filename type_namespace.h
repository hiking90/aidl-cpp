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

struct user_data_type;
struct interface_type;

namespace android {
namespace aidl {

class TypeNamespace {
 public:
  // Load this TypeNamespace with user defined types.
  virtual bool AddParcelableType(const user_data_type* p,
                                 const std::string& filename) = 0;
  virtual bool AddBinderType(const interface_type* b,
                             const std::string& filename) = 0;
  virtual bool AddContainerType(const std::string& type_name) = 0;

  // Search for a type by inexact match with |name|.
  virtual const Type* Search(const std::string& name) = 0;
  // Search for a type by exact match with |name|.
  virtual const Type* Find(const std::string& name) const = 0;

 protected:
  TypeNamespace() = default;
  virtual ~TypeNamespace() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(TypeNamespace);
};

}  // namespace aidl
}  // namespace android

#endif  // AIDL_TYPE_NAMESPACE_H_
