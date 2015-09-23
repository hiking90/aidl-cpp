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

#include "generate_cpp.h"
#include "parse_helpers.h"

#include <cctype>
#include <memory>
#include <random>
#include <string>

#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "logging.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace internals {

string StrippedLiteral(const buffer_type& buffer) {
  std::string i_name = buffer.Literal();

  string c_name;

  if (i_name.length() >= 2 && i_name[0] == 'I' && isupper(i_name[1]))
    c_name = i_name.substr(1);
  else
    c_name = i_name;

  return c_name;
}

string GetCPPType(type_type *type) {
  string lit = type->type.Literal();

  if (lit == "int")
    return "int32_t";
  else if (lit == "long")
    return "int64_t";
  else
    LOG(FATAL) << "Type not yet supported";

  return "int";
}

string GetCPPVarDec(type_type* type, string name,
                    int direction) {
  return GetCPPType(type) + ((OUT_PARAMETER & direction) ? "* " : " ") +
      name + type->Brackets();
}

unique_ptr<CppDocument> BuildClientSource(interface_type* parsed_doc) {
  unique_ptr<CppNamespace> ns{new CppNamespace{"android", {}}};
  return unique_ptr<CppDocument>{new CppSource{ {}, std::move(ns)}};
}

unique_ptr<CppDocument> BuildServerSource(interface_type* parsed_doc) {
  unique_ptr<CppNamespace> ns{new CppNamespace{"android", {}}};
  return unique_ptr<CppDocument>{new CppSource{ {}, std::move(ns)}};
}

unique_ptr<CppDocument> BuildInterfaceSource(interface_type* parsed_doc) {
  unique_ptr<CppNamespace> ns{new CppNamespace{"android", {}}};
  return unique_ptr<CppDocument>{new CppSource{ {}, std::move(ns)}};
}

unique_ptr<CppDocument> BuildClientHeader(interface_type* parsed_doc) {
  string i_name = parsed_doc->name.Literal();
  string c_name = StrippedLiteral(parsed_doc->name);
  string bp_name = "Bp" + c_name;

  unique_ptr<CppMacroOrConstructorDeclaration> constructor{
        new CppMacroOrConstructorDeclaration(bp_name, {}, false, false)};

  unique_ptr<CppMacroOrConstructorDeclaration> destructor{
        new CppMacroOrConstructorDeclaration("~" + bp_name, {}, false, false)};

  vector<unique_ptr<CppDeclaration>> publics;
  publics.push_back(std::move(constructor));
  publics.push_back(std::move(destructor));

  for (interface_item_type *item = parsed_doc->interface_items;
       item;
       item = item->next) {
    if (item->item_type != METHOD_TYPE) {
      LOG(FATAL) << "Unknown interface item type";
      return nullptr;
    }

    method_type *m_item = (method_type *)item;

    string method_name = m_item->name.Literal();
    string return_arg = GetCPPVarDec(&m_item->type, "_aidl_return",
                                     OUT_PARAMETER);

    vector<string> args;

    for (arg_type *arg = m_item->args; arg; arg = arg->next)
      args.push_back(GetCPPVarDec(&arg->type, arg->name.Literal(),
                                  convert_direction(arg->direction.data)));

    args.push_back(return_arg);

    unique_ptr<CppDeclaration> meth {
        new CppMethodDeclaration("android::status_t", method_name,
                                 args, false, true)
    };

    publics.push_back(std::move(meth));
  }

  unique_ptr<CppClassDeclaration> bp_class{
      new CppClassDeclaration{bp_name,
                              "public android::BpInterface<" + i_name + ">",
                              std::move(publics),
                              {}
      }};

  vector<unique_ptr<CppDeclaration>> bp_class_vec;
  bp_class_vec.push_back(std::move(bp_class));

  unique_ptr<CppNamespace> ns {new CppNamespace{"android", std::move(bp_class_vec)}};

  unique_ptr<CppDocument> bp_header{new CppHeader{bp_name + "_H",
                                                  {"binder/IBinder.h",
                                                   "binder/IInterface.h",
                                                   "utils/Errors.h",
                                                   i_name + ".h"},
                                                  std::move(ns) }};

  return bp_header;
}

unique_ptr<CppDocument> BuildServerHeader(interface_type* parsed_doc) {
  string i_name = parsed_doc->name.Literal();;
  string c_name = StrippedLiteral(parsed_doc->name);
  string bn_name = "Bn" + c_name;

  unique_ptr<CppDeclaration> on_transact{
      new CppMethodDeclaration("android::status_t", "onTransact",
                               { "uint32_t code",
                                 "const android::Parcel& data",
                                 "android::Parcel* reply",
                                 "uint32_t flags = 0"
                               })};

  std::vector<unique_ptr<CppDeclaration>> publics;
  publics.push_back(std::move(on_transact));

  unique_ptr<CppClassDeclaration> bn_class{
      new CppClassDeclaration{bn_name,
                              "public android::BnInterface<" + i_name + ">",
                              std::move(publics),
                              {}
      }};

  std::vector<unique_ptr<CppDeclaration>> declarations;
  declarations.push_back(std::move(bn_class));

  unique_ptr<CppNamespace> ns{new CppNamespace{"android", std::move(declarations)}};

  unique_ptr<CppDocument> bn_header{new CppHeader{bn_name + "_H",
                                                  {"binder/IInterface.h",
                                                   i_name + ".h"},
                                                  std::move(ns) }};

  return bn_header;
}

unique_ptr<CppDocument> BuildInterfaceHeader(interface_type* parsed_doc) {
  unique_ptr<CppNamespace> ns{new CppNamespace{"android", {}}};
  return unique_ptr<CppDocument>{new CppSource{ {}, std::move(ns)}};
}

bool GenerateCppForFile(const std::string& name, unique_ptr<CppDocument> doc) {
  if (!doc) {
    return false;
  }
  unique_ptr<CodeWriter> writer = GetFileWriter(name);
  doc->Write(writer.get());
  return true;
}

}  // namespace internals

using namespace internals;

bool GenerateCpp(const CppOptions& options, interface_type* parsed_doc) {
  bool success = true;

  success &= GenerateCppForFile(options.ClientCppFileName(),
                                BuildClientSource(parsed_doc));
  success &= GenerateCppForFile(options.ClientHeaderFileName(),
                                BuildClientHeader(parsed_doc));
  success &= GenerateCppForFile(options.ServerCppFileName(),
                                BuildServerSource(parsed_doc));
  success &= GenerateCppForFile(options.ServerHeaderFileName(),
                                BuildServerHeader(parsed_doc));
  success &= GenerateCppForFile(options.InterfaceCppFileName(),
                                BuildInterfaceSource(parsed_doc));
  success &= GenerateCppForFile(options.InterfaceHeaderFileName(),
                                BuildInterfaceHeader(parsed_doc));

  return success;
}

}  // namespace android
}  // namespace aidl
