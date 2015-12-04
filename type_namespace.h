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

#include <android-base/macros.h>

#include "aidl_language.h"
#include "logging.h"

namespace android {
namespace aidl {

class ValidatableType {
 public:
  enum {
    KIND_BUILT_IN,
    KIND_PARCELABLE,
    KIND_INTERFACE,
    KIND_GENERATED,
  };

  ValidatableType(int kind,
                  const std::string& package, const std::string& type_name,
                  const std::string& decl_file, int decl_line);
  virtual ~ValidatableType() = default;

  virtual bool CanBeArray() const = 0;
  virtual bool CanBeOutParameter() const = 0;
  virtual bool CanWriteToParcel() const = 0;

  // Name() returns the short name of an object (without package qualifiers).
  virtual std::string Name() const { return type_name_; }
  // QualifiedName() returns the canonical AIDL type, with packages.
  virtual std::string QualifiedName() const { return canonical_name_; }
  int Kind() const { return kind_; }
  std::string HumanReadableKind() const;
  std::string DeclFile() const { return origin_file_; }
  int DeclLine() const { return origin_line_; }

 private:
  const int kind_;
  const std::string type_name_;
  const std::string canonical_name_;
  const std::string origin_file_;
  const int origin_line_;

  DISALLOW_COPY_AND_ASSIGN(ValidatableType);
};

class TypeNamespace {
 public:
  // Load the TypeNamespace with built in types.  Don't do work in the
  // constructor because many of the useful methods are virtual.
  virtual void Init() = 0;

  // Load this TypeNamespace with user defined types.
  virtual bool AddParcelableType(const AidlParcelable& p,
                                 const std::string& filename) = 0;
  virtual bool AddBinderType(const AidlInterface& b,
                             const std::string& filename) = 0;
  // We dynamically create container types as we discover them in the parse
  // tree.  Returns false if the contained types cannot be canonicalized.
  virtual bool AddListType(const std::string& contained_type_name) = 0;
  virtual bool AddMapType(const std::string& key_type_name,
                          const std::string& value_type_name) = 0;

  // Add a container type to this namespace.  Returns false only
  // on error. Silently discards requests to add non-container types.
  virtual bool MaybeAddContainerType(const std::string& type_name);

  // Returns true iff this has a type for |type_name|.
  virtual bool HasType(const std::string& type_name) const;

  // Returns true iff |package| is a valid package name.
  virtual bool IsValidPackage(const std::string& package) const;

  // Returns true iff |raw_type| is a valid return type.
  virtual bool IsValidReturnType(const AidlType& raw_type,
                                 const std::string& filename) const;

  // Returns true iff |arg_type| is a valid method argument.
  virtual bool IsValidArg(const AidlArgument& a,
                          int arg_index,
                          const std::string& filename) const;

  // Returns true if this is a container type, rather than a normal type.
  virtual bool IsContainerType(const std::string& type_name) const;

  // Returns true iff the name can be canonicalized to a container type.
  virtual bool CanonicalizeContainerType(
      const std::string& raw_type_name,
      std::vector<std::string>* container_class,
      std::vector<std::string>* contained_type_names) const;

 protected:
  TypeNamespace() = default;
  virtual ~TypeNamespace() = default;

  // Get a pointer to an existing type.
  virtual const ValidatableType* GetValidatableType(
      const std::string& name) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TypeNamespace);
};

template<typename T>
class LanguageTypeNamespace : public TypeNamespace {
 public:
  LanguageTypeNamespace() = default;
  virtual ~LanguageTypeNamespace() = default;

  // Get a pointer to an existing type.  Searches first by fully-qualified
  // name, and then class name (dropping package qualifiers).
  const T* Find(const std::string& name) const;

 protected:
  bool Add(const T* type);
  const ValidatableType* GetValidatableType(
      const std::string& name) const override { return Find(name); }

 private:
  std::vector<std::unique_ptr<const T>> types_;

  DISALLOW_COPY_AND_ASSIGN(LanguageTypeNamespace);
};  // class LanguageTypeNamespace

template<typename T>
bool LanguageTypeNamespace<T>::Add(const T* type) {
  const T* existing = Find(type->QualifiedName());
  if (!existing) {
    types_.emplace_back(type);
    return true;
  }

  if (existing->Kind() == ValidatableType::KIND_BUILT_IN) {
    LOG(ERROR) << type->DeclFile() << ":" << type->DeclLine()
               << " attempt to redefine built in class "
               << type->QualifiedName();
    return false;
  }

  if (type->Kind() != existing->Kind()) {
    LOG(ERROR) << type->DeclFile() << ":" << type->DeclLine()
               << " attempt to redefine " << type->QualifiedName()
               << " as " << type->HumanReadableKind();
    LOG(ERROR) << existing->DeclFile() << ":" << existing->DeclLine()
               << " previously defined here as "
               << existing->HumanReadableKind();
    return false;
  }

  return true;
}

template<typename T>
const T* LanguageTypeNamespace<T>::Find(const std::string& raw_name) const {
  using std::string;
  using std::vector;
  using android::base::Join;
  using android::base::Trim;

  string name = Trim(raw_name);
  if (IsContainerType(name)) {
    vector<string> container_class;
    vector<string> contained_type_names;
    if (!CanonicalizeContainerType(name, &container_class,
                                   &contained_type_names)) {
      return nullptr;
    }
    for (string& contained_type_name : contained_type_names) {
      const T* contained_type = Find(contained_type_name);
      if (!contained_type) {
        return nullptr;
      }
      contained_type_name = contained_type->QualifiedName();
    }
    name = Join(container_class, '.') +
           "<" + Join(contained_type_names, ',') + ">";
  }

  const T* ret = nullptr;
  for (const auto& type : types_) {
    // Always prefer a exact match if possible.
    // This works for primitives and class names qualified with a package.
    if (type->QualifiedName() == name) {
      ret = type.get();
      break;
    }
    // We allow authors to drop packages when refering to a class name.
    if (type->Name() == name) {
      ret = type.get();
    }
  }

  return ret;
}

}  // namespace aidl
}  // namespace android

#endif  // AIDL_TYPE_NAMESPACE_H_
