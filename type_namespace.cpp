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

#include "type_namespace.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "aidl_language.h"
#include "logging.h"

using android::base::StringPrintf;
using android::base::Split;
using android::base::Trim;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

namespace android {
namespace aidl {

namespace {

bool is_java_keyword(const char* str) {
  static const std::vector<std::string> kJavaKeywords{
      "abstract",   "assert",       "boolean",   "break",      "byte",
      "case",       "catch",        "char",      "class",      "const",
      "continue",   "default",      "do",        "double",     "else",
      "enum",       "extends",      "final",     "finally",    "float",
      "for",        "goto",         "if",        "implements", "import",
      "instanceof", "int",          "interface", "long",       "native",
      "new",        "package",      "private",   "protected",  "public",
      "return",     "short",        "static",    "strictfp",   "super",
      "switch",     "synchronized", "this",      "throw",      "throws",
      "transient",  "try",          "void",      "volatile",   "while",
      "true",       "false",        "null",
  };
  return std::find(kJavaKeywords.begin(), kJavaKeywords.end(), str) !=
      kJavaKeywords.end();
}

} // namespace

ValidatableType::ValidatableType(
    int kind, const string& package, const string& type_name,
    const string& decl_file, int decl_line)
    : kind_(kind),
      type_name_(type_name),
      canonical_name_((package.empty()) ? type_name
                                        : package + "." + type_name),
      origin_file_(decl_file),
      origin_line_(decl_line) {}

string ValidatableType::HumanReadableKind() const {
  switch (Kind()) {
    case ValidatableType::KIND_BUILT_IN:
      return "a built in";
    case ValidatableType::KIND_PARCELABLE:
      return "a parcelable";
    case ValidatableType::KIND_INTERFACE:
      return "an interface";
    case ValidatableType::KIND_GENERATED:
      return "a generated";
  }
  return "unknown";
}

bool TypeNamespace::MaybeAddContainerType(const std::string& type_name) {
  if (!IsContainerType(type_name) || HasType(type_name)) {
    return true;
  }

  vector<string> container_class;
  vector<string> contained_type_names;
  if (!CanonicalizeContainerType(type_name, &container_class,
                                 &contained_type_names)) {
    return false;
  }

  // We only support two types right now and this type is one of them.
  switch (contained_type_names.size()) {
    case 1:
      return AddListType(contained_type_names[0]);
    case 2:
      return AddMapType(contained_type_names[0], contained_type_names[1]);
    default:
      break;  // Should never get here, will FATAL below.
  }

  LOG(FATAL) << "aidl internal error";
  return false;
}


bool TypeNamespace::IsContainerType(const string& type_name) const {
  const size_t opening_brace = type_name.find('<');
  const size_t closing_brace = type_name.find('>');
  if (opening_brace != string::npos || closing_brace != string::npos) {
    return true;  // Neither < nor > appear in normal AIDL types.
  }
  return false;
}

bool TypeNamespace::CanonicalizeContainerType(
    const string& raw_type_name,
    vector<string>* container_class,
    vector<string>* contained_type_names) const {
  string name = Trim(raw_type_name);
  const size_t opening_brace = name.find('<');
  const size_t closing_brace = name.find('>');
  if (opening_brace == string::npos || closing_brace == string::npos) {
    return false;
  }

  if (opening_brace != name.rfind('<') ||
      closing_brace != name.rfind('>') ||
      closing_brace != name.length() - 1) {
    // Nested/invalid templates are forbidden.
    LOG(ERROR) << "Invalid template type '" << name << "'";
    return false;
  }

  string container = Trim(name.substr(0, opening_brace));
  string remainder = name.substr(opening_brace + 1,
                                 (closing_brace - opening_brace) - 1);
  vector<string> args = Split(remainder, ",");

  // Map the container name to its canonical form for supported containers.
  if ((container == "List" || container == "java.util.List") &&
      args.size() == 1) {
    *container_class = {"java", "util", "List"};
    *contained_type_names = args;
    return true;
  }
  if ((container == "Map" || container == "java.util.Map") &&
      args.size() == 2) {
    *container_class = {"java", "util", "Map"};
    *contained_type_names = args;
    return true;
  }

  LOG(ERROR) << "Unknown find container with name " << container
             << " and " << args.size() << "contained types.";
  return false;
}

bool TypeNamespace::HasType(const string& type_name) const {
  return GetValidatableType(type_name) != nullptr;
}

bool TypeNamespace::IsValidPackage(const string& /* package */) const {
  return true;
}

const ValidatableType* TypeNamespace::GetReturnType(const AidlType& raw_type,
                           const string& filename) const {
  const string error_prefix = StringPrintf(
      "In file %s line %d return type %s:\n    ",
      filename.c_str(), raw_type.GetLine(), raw_type.ToString().c_str());

  const ValidatableType* return_type = GetValidatableType(raw_type.GetName());
  if (return_type == nullptr) {
    cerr << error_prefix << "unknown return type" << endl;
    return nullptr;
  }

  if (raw_type.GetName() != "void" && !return_type->CanWriteToParcel()) {
    cerr << error_prefix << "return type cannot be marshalled" << endl;
    return nullptr;
  }

  if (raw_type.IsArray()) {
      return_type = return_type->ArrayType();
  }

  if (!return_type) {
    cerr << error_prefix << "return type cannot be an array" << endl;
    return nullptr;
  }

  if (raw_type.IsNullable()) {
    return_type = return_type->NullableType();
  }

  if (!return_type) {
    cerr << error_prefix << "return type cannot be nullable" << endl;
    return nullptr;
  }

  return return_type;
}

const ValidatableType* TypeNamespace::GetArgType(const AidlArgument& a,
                           int arg_index,
                           const string& filename) const {
  string error_prefix = StringPrintf(
      "In file %s line %d parameter %s (%d):\n    ",
      filename.c_str(), a.GetLine(), a.GetName().c_str(), arg_index);

  // check the arg type
  const ValidatableType* t = GetValidatableType(a.GetType().GetName());
  if (t == nullptr) {
    cerr << error_prefix << "unknown type " << a.GetType().GetName().c_str()
         << endl;
    return nullptr;
  }

  if (!t->CanWriteToParcel()) {
    cerr << error_prefix
         << StringPrintf("'%s' can't be marshalled.",
                         a.GetType().ToString().c_str()) << endl;
    return nullptr;
  }

  if (a.GetType().IsArray()) {
    t = t->ArrayType();
  }

  if (!t) {
    cerr << error_prefix << StringPrintf(
        "'%s' cannot be an array.",
        a.ToString().c_str()) << endl;
    return nullptr;
  }

  if (a.GetType().IsNullable()) {
    t = t->NullableType();
  }

  if (!t) {
    cerr << error_prefix << StringPrintf(
        "'%s' cannot be nullable.",
        a.ToString().c_str()) << endl;
    return nullptr;
  }

  if (!a.DirectionWasSpecified() && t->CanBeOutParameter()) {
    cerr << error_prefix << StringPrintf(
        "'%s' can be an out type, so you must declare it as in,"
        " out or inout.",
        a.GetType().ToString().c_str()) << endl;
    return nullptr;
  }

  if (a.GetDirection() != AidlArgument::IN_DIR &&
      !t->CanBeOutParameter()) {
    cerr << error_prefix << StringPrintf(
        "'%s' can only be an in parameter.",
        a.ToString().c_str()) << endl;
    return nullptr;
  }

  // check that the name doesn't match a keyword
  if (is_java_keyword(a.GetName().c_str())) {
    cerr << error_prefix << "Argument name is a Java or aidl keyword"
         << endl;
    return nullptr;
  }

  // Reserve a namespace for internal use
  if (a.GetName().substr(0, 5)  == "_aidl") {
    cerr << error_prefix << "Argument name cannot begin with '_aidl'"
         << endl;
    return nullptr;
  }

  return t;
}

}  // namespace aidl
}  // namespace android
