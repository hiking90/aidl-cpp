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

#include <base/stringprintf.h>

#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "logging.h"

using android::base::StringPrintf;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace cpp {
namespace internals {
namespace {

const char kAndroidStatusLiteral[] = "android::status_t";
const char kIBinderHeader[] = "binder/IBinder.h";
const char kIInterfaceHeader[] = "binder/IInterface.h";

string UpperCase(const std::string& s) {
  string result = s;
  for (char& c : result)
    c = toupper(c);
  return result;
}

string GetCPPVarDec(const TypeNamespace& types, const AidlType& type,
                    const string& var_name, bool use_pointer) {
  const Type* cpp_type = types.Find(type.GetName());
  if (cpp_type == nullptr) {
    // We should have caught this in type resolution.
    LOG(FATAL) << "internal error";
  }
  return StringPrintf("%s%s %s%s",
                      cpp_type->CppType().c_str(),
                      (use_pointer) ? "*" : "",
                      var_name.c_str(),
                      type.Brackets().c_str());
}

unique_ptr<Declaration> BuildMethodDecl(const AidlMethod& method,
                                        const TypeNamespace& types,
                                        bool for_interface) {
  vector<string> args;
  for (const unique_ptr<AidlArgument>& arg : method.GetArguments()) {
    args.push_back(GetCPPVarDec(
          types, arg->GetType(), arg->GetName(),
          AidlArgument::OUT_DIR & arg->GetDirection()));
  }

  string return_arg = GetCPPVarDec(types, method.GetType(), "_aidl_return", true);
  args.push_back(return_arg);

  uint32_t modifiers = 0;
  if (for_interface) {
    modifiers |= MethodDecl::IS_VIRTUAL;
    modifiers |= MethodDecl::IS_PURE_VIRTUAL;
  } else {
    modifiers |= MethodDecl::IS_OVERRIDE;
  }

  return unique_ptr<Declaration>{
      new MethodDecl{kAndroidStatusLiteral,
                     method.GetName(),
                     args,
                     modifiers}};
}

unique_ptr<CppNamespace> NestInNamespaces(unique_ptr<Declaration> decl) {
  using N = CppNamespace;
  using NPtr = unique_ptr<N>;
  return NPtr{new N{"android", NPtr{new N{"generated", std::move(decl)}}}};
}

}  // namespace

enum class ClassNames { BASE, CLIENT, SERVER, INTERFACE };

string ClassName(const interface_type& interface, ClassNames type) {
  string c_name = interface.name.Literal();

  if (c_name.length() >= 2 && c_name[0] == 'I' && isupper(c_name[1]))
    c_name = c_name.substr(1);

  switch (type) {
    case ClassNames::CLIENT:
      c_name = "Bp" + c_name;
      break;
    case ClassNames::SERVER:
      c_name = "Bn" + c_name;
      break;
    case ClassNames::INTERFACE:
      c_name = "I" + c_name;
      break;
    case ClassNames::BASE:
      break;
  }
  return c_name;
}

unique_ptr<Document> BuildClientSource(const TypeNamespace& types,
                                       const interface_type& parsed_doc) {
  unique_ptr<CppNamespace> ns{new CppNamespace{"android"}};
  return unique_ptr<Document>{new CppSource{ {}, std::move(ns)}};
}

unique_ptr<Document> BuildServerSource(const TypeNamespace& types,
                                       const interface_type& parsed_doc) {
  unique_ptr<CppNamespace> ns{new CppNamespace{"android"}};
  return unique_ptr<Document>{new CppSource{ {}, std::move(ns)}};
}

unique_ptr<Document> BuildInterfaceSource(const TypeNamespace& types,
                                          const interface_type& parsed_doc) {
  const string i_name = ClassName(parsed_doc, ClassNames::INTERFACE);
  const string bp_name = ClassName(parsed_doc, ClassNames::CLIENT);
  vector<string> include_list{i_name + ".h", bp_name + ".h"};

  string fq_name = i_name;
  if (parsed_doc.package != nullptr && strlen(parsed_doc.package) > 0) {
    fq_name = StringPrintf("%s.%s", parsed_doc.package, i_name.c_str());
  }

  unique_ptr<ConstructorDecl> meta_if{new ConstructorDecl{
      "IMPLEMENT_META_INTERFACE",
      {ClassName(parsed_doc, ClassNames::BASE), '"' + fq_name + '"'}
  }};

  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(meta_if))}};
}

unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                       const interface_type& parsed_doc) {
  const string i_name = ClassName(parsed_doc, ClassNames::INTERFACE);
  const string bp_name = ClassName(parsed_doc, ClassNames::CLIENT);

  unique_ptr<ConstructorDecl> constructor{new ConstructorDecl(bp_name, {})};
  unique_ptr<ConstructorDecl> destructor{
        new ConstructorDecl("~" + bp_name, {})};

  vector<unique_ptr<Declaration>> publics;
  publics.push_back(std::move(constructor));
  publics.push_back(std::move(destructor));

  for (const auto& item : *parsed_doc.methods) {
    publics.push_back(BuildMethodDecl(*item, types, false));
  }

  unique_ptr<ClassDecl> bp_class{
      new ClassDecl{bp_name,
                    "public android::BpInterface<" + i_name + ">",
                    std::move(publics),
                    {}
      }};

  return unique_ptr<Document>{new CppHeader{
      bp_name + "_H",
      {kIBinderHeader,
       kIInterfaceHeader,
       "utils/Errors.h",
       i_name + ".h"},
      NestInNamespaces(std::move(bp_class))}};
}

unique_ptr<Document> BuildServerHeader(const TypeNamespace& types,
                                       const interface_type& parsed_doc) {
  const string i_name = ClassName(parsed_doc, ClassNames::INTERFACE);
  const string bn_name = ClassName(parsed_doc, ClassNames::SERVER);

  unique_ptr<Declaration> on_transact{
      new MethodDecl(kAndroidStatusLiteral, "onTransact",
                     { "uint32_t code",
                       "const android::Parcel& data",
                       "android::Parcel* reply",
                       "uint32_t flags = 0"
                     })};

  std::vector<unique_ptr<Declaration>> publics;
  publics.push_back(std::move(on_transact));

  unique_ptr<ClassDecl> bn_class{
      new ClassDecl{bn_name,
                    "public android::BnInterface<" + i_name + ">",
                    std::move(publics),
                    {}
      }};

  return unique_ptr<Document>{new CppHeader{
      bn_name + "_H",
      {"binder/IInterface.h",
       i_name + ".h"},
      NestInNamespaces(std::move(bn_class))}};
}

unique_ptr<Document> BuildInterfaceHeader(const TypeNamespace& types,
                                          const interface_type& parsed_doc) {
  unique_ptr<ClassDecl> if_class{
      new ClassDecl{ClassName(parsed_doc, ClassNames::INTERFACE),
                    "public android::IInterface"}};
  if_class->AddPublic(unique_ptr<Declaration>{
      new ConstructorDecl{
          "DECLARE_META_INTERFACE",
          {ClassName(parsed_doc, ClassNames::BASE)}}});

  unique_ptr<Enum> call_enum{new Enum{"Call"}};
  for (const auto& method : *parsed_doc.methods) {
    if_class->AddPublic(BuildMethodDecl(*method, types, true));
    call_enum->AddValue(
        UpperCase(method->GetName()),
        StringPrintf("android::IBinder::FIRST_CALL_TRANSACTION + %d",
                     method->GetId()));
  }
  if_class->AddPublic(std::move(call_enum));

  return unique_ptr<Document>{new CppSource{
      {kIBinderHeader,
       kIInterfaceHeader},
      NestInNamespaces(std::move(if_class))}};
}

bool GenerateCppForFile(const std::string& name, unique_ptr<Document> doc) {
  if (!doc) {
    return false;
  }
  unique_ptr<CodeWriter> writer = GetFileWriter(name);
  doc->Write(writer.get());
  return true;
}

}  // namespace internals

using namespace internals;

bool GenerateCpp(const CppOptions& options,
                 const TypeNamespace& types,
                 const interface_type& parsed_doc) {
  bool success = true;

  success &= GenerateCppForFile(options.ClientCppFileName(),
                                BuildClientSource(types, parsed_doc));
  success &= GenerateCppForFile(options.ClientHeaderFileName(),
                                BuildClientHeader(types, parsed_doc));
  success &= GenerateCppForFile(options.ServerCppFileName(),
                                BuildServerSource(types, parsed_doc));
  success &= GenerateCppForFile(options.ServerHeaderFileName(),
                                BuildServerHeader(types, parsed_doc));
  success &= GenerateCppForFile(options.InterfaceCppFileName(),
                                BuildInterfaceSource(types, parsed_doc));
  success &= GenerateCppForFile(options.InterfaceHeaderFileName(),
                                BuildInterfaceHeader(types, parsed_doc));

  return success;
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
