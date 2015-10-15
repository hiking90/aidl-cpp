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

#include <cctype>
#include <cstring>
#include <memory>
#include <random>
#include <string>

#include <base/stringprintf.h>
#include <base/strings.h>

#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "logging.h"

using android::base::StringPrintf;
using android::base::Join;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace cpp {
namespace internals {
namespace {

const char kStatusOkOrBreakCheck[] = "if (status != android::OK) { break; }";
const char kStatusOkOrReturnLiteral[] =
    "if (status != android::OK) { return status; }";
const char kReturnVarName[] = "_aidl_return";
const char kAndroidStatusLiteral[] = "android::status_t";
const char kAndroidParcelLiteral[] = "android::Parcel";
const char kIBinderHeader[] = "binder/IBinder.h";
const char kIInterfaceHeader[] = "binder/IInterface.h";
const char kParcelHeader[] = "binder/Parcel.h";

string UpperCase(const std::string& s) {
  string result = s;
  for (char& c : result)
    c = toupper(c);
  return result;
}

string BuildVarName(const AidlArgument& a) {
  string prefix = "out_";
  if (a.GetDirection() & AidlArgument::IN_DIR) {
    prefix = "in_";
  }
  return prefix + a.GetName();
}

ArgList BuildArgList(const TypeNamespace& types,
                     const AidlMethod& method,
                     bool for_declaration) {
  // Build up the argument list for the server method call.
  vector<string> method_arguments;
  for (const unique_ptr<AidlArgument>& a : method.GetArguments()) {
    string literal;
    if (for_declaration) {
      // Method declarations need types, pointers to out params, and variable
      // names that match the .aidl specification.
      const Type* type = types.Find(a->GetType().GetName());
      literal = StringPrintf(
          "%s%s %s", type->CppType().c_str(),
          (a->IsOut()) ? "*" : "",
          a->GetName().c_str());
    } else {
      if (a->IsOut()) { literal = "&"; }
      literal += BuildVarName(*a);
    }
    method_arguments.push_back(literal);
  }

  const Type* return_type = types.Find(method.GetType().GetName());
  if (return_type != types.VoidType()) {
    string literal;
    if (for_declaration) {
      literal = StringPrintf(
          "%s* %s", return_type->CppType().c_str(), kReturnVarName);
    } else {
      literal = string{"&"} + kReturnVarName;
    }
    method_arguments.push_back(literal);
  }

  return ArgList(method_arguments);
}

unique_ptr<Declaration> BuildMethodDecl(const AidlMethod& method,
                                        const TypeNamespace& types,
                                        bool for_interface) {
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
                     BuildArgList(types, method, true /* for method decl */),
                     modifiers}};
}

unique_ptr<CppNamespace> NestInNamespaces(unique_ptr<Declaration> decl) {
  using N = CppNamespace;
  using NPtr = unique_ptr<N>;
  return NPtr{new N{"android", NPtr{new N{"generated", std::move(decl)}}}};
}

unique_ptr<CppNamespace> NestInNamespaces(
    vector<unique_ptr<Declaration>> decls) {
  using N = CppNamespace;
  using NPtr = unique_ptr<N>;
  return NPtr{new N{"android", NPtr{new N{"generated", std::move(decls)}}}};
}

bool DeclareLocalVariable(const TypeNamespace& types, const AidlArgument& a,
                          StatementBlock* b) {
  const Type* cpp_type = types.Find(a.GetType().GetName());
  if (!cpp_type) { return false; }

  b->AddLiteral(cpp_type->CppType() + " " + BuildVarName(a));
  return true;
}

enum class ClassNames { BASE, CLIENT, SERVER, INTERFACE };

string ClassName(const AidlInterface& interface, ClassNames type) {
  string c_name = interface.GetName();

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

string BuildHeaderGuard(const AidlInterface& interface,
                        ClassNames header_type) {
  string class_name = ClassName(interface, header_type);
  for (size_t i = 1; i < class_name.size(); ++i) {
    if (isupper(class_name[i])) {
      class_name.insert(i, "_");
      ++i;
    }
  }
  string ret = StringPrintf("AIDL_GENERATED_%s_%s_H_",
                            interface.GetPackage().c_str(),
                            class_name.c_str());
  for (char& c : ret) {
    if (c == '.') {
      c = '_';
    }
    c = toupper(c);
  }
  return ret;
}

unique_ptr<Declaration> DefineClientTransaction(const TypeNamespace& types,
                                                const AidlInterface& interface,
                                                const AidlMethod& method) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bp_name = ClassName(interface, ClassNames::CLIENT);
  unique_ptr<MethodImpl> ret{new MethodImpl{
      kAndroidStatusLiteral, bp_name, method.GetName(),
      ArgList{BuildArgList(types, method, true /* for method decl */)}}};
  StatementBlock* b = ret->GetStatementBlock();

  // Declare parcels to hold our query and the response.
  b->AddLiteral(StringPrintf("%s data", kAndroidParcelLiteral));
  b->AddLiteral(StringPrintf("%s reply", kAndroidParcelLiteral));
  // And declare the status variable we need for error handling.
  b->AddLiteral(StringPrintf("%s status", kAndroidStatusLiteral));

  // Serialization looks roughly like:
  //     status = data.WriteInt32(in_param_name);
  //     if (status != android::OK) { return status; }
  for (const AidlArgument* a : method.GetInArguments()) {
    string method = types.Find(a->GetType().GetName())->WriteToParcelMethod();
    string var_name = ((a->IsOut()) ? "*" : "") + a->GetName();
    b->AddStatement(new Assignment(
        "status",
        new MethodCall("data." + method, ArgList(var_name))));
    b->AddLiteral(kStatusOkOrReturnLiteral, false /* no semicolon */);
  }

  // Invoke the transaction on the remote binder and confirm status.
  string transaction_code = StringPrintf(
      "%s::%s", i_name.c_str(), UpperCase(method.GetName()).c_str());
  b->AddStatement(new Assignment(
      "status",
      new MethodCall("remote()->transact",
                     ArgList({transaction_code, "data", "&reply"}))));
  b->AddLiteral(kStatusOkOrReturnLiteral, false /* no semicolon */);

  // If the method is expected to return something, read it first by convention.
  const Type* return_type = types.Find(method.GetType().GetName());
  if (return_type != types.VoidType()) {
    string method = return_type->ReadFromParcelMethod();
    b->AddStatement(new Assignment(
        "status",
        new MethodCall("reply." + method, ArgList(kReturnVarName))));
    b->AddLiteral(kStatusOkOrReturnLiteral, false /* no semicolon */);
  }

  for (const AidlArgument* a : method.GetOutArguments()) {
    // Deserialization looks roughly like:
    //     status = reply.ReadInt32(out_param_name);
    //     if (status != android::OK) { return status; }
    string method = types.Find(a->GetType().GetName())->ReadFromParcelMethod();
    b->AddStatement(new Assignment(
        "status",
        new MethodCall("reply." + method, ArgList(a->GetName()))));
    b->AddLiteral(kStatusOkOrReturnLiteral, false /* no semicolon */);
  }

  b->AddLiteral("return status");

  return unique_ptr<Declaration>(ret.release());
}

}  // namespace

unique_ptr<Document> BuildClientSource(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  const string bp_name = ClassName(interface, ClassNames::CLIENT);
  vector<string> include_list = { bp_name + ".h", kParcelHeader };
  vector<unique_ptr<Declaration>> file_decls;

  // The constructor just passes the IBinder instance up to the super
  // class.
  file_decls.push_back(unique_ptr<Declaration>{new ConstructorImpl{
      bp_name,
      ArgList{"const android::sp<android::IBinder>& impl"},
      { "BpInterface<IPingResponder>(impl)" }}});

  // Clients define a method per transaction.
  for (const auto& method : interface.GetMethods()) {
    unique_ptr<Declaration> m = DefineClientTransaction(
        types, interface, *method);
    if (!m) { return nullptr; }
    file_decls.push_back(std::move(m));
  }
  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(file_decls))}};
}

namespace {

bool HandleServerTransaction(const TypeNamespace& types,
                             const AidlMethod& method,
                             StatementBlock* b) {
  // Declare all the parameters now.  In the common case, we expect no errors
  // in serialization.
  for (const unique_ptr<AidlArgument>& a : method.GetArguments()) {
    if (!DeclareLocalVariable(types, *a, b)) { return false; }
  }

  // Declare a variable to hold the return value.
  const Type* return_type = types.Find(method.GetType().GetName());
  if (return_type != types.VoidType()) {
    b->AddLiteral(StringPrintf(
        "%s %s", return_type->CppType().c_str(), kReturnVarName));
  }

  // Deserialize each "in" parameter to the transaction.
  for (const AidlArgument* a : method.GetInArguments()) {
    // Deserialization looks roughly like:
    //     status = data.ReadInt32(&in_param_name);
    //     if (status != android::OK) { break; }
    const Type* type = types.Find(a->GetType().GetName());
    b->AddStatement(new Assignment{
        "status",
        new MethodCall{"data." + type->ReadFromParcelMethod(),
                       "&" + BuildVarName(*a)}});
    b->AddLiteral(kStatusOkOrBreakCheck, false /* no semicolon */);
  }

  // Call the actual method.  This is implemented by the subclass.
  b->AddStatement(new Assignment{
      "status", new MethodCall{
          method.GetName(),
          BuildArgList(types, method, false /* not for method decl */)}});
  b->AddLiteral(kStatusOkOrBreakCheck, false /* no semicolon */);

  // If we have a return value, write it first.
  if (return_type != types.VoidType()) {
    string method = "reply->" + return_type->WriteToParcelMethod();
    b->AddStatement(new Assignment{
        "status", new MethodCall{method, ArgList{kReturnVarName}}});
    b->AddLiteral(kStatusOkOrBreakCheck, false /* no semicolon */);
  }

  // Write each out parameter to the reply parcel.
  for (const AidlArgument* a : method.GetOutArguments()) {
    // Serialization looks roughly like:
    //     status = data.WriteInt32(out_param_name);
    //     if (status != android::OK) { break; }
    const Type* type = types.Find(a->GetType().GetName());
    b->AddStatement(new Assignment{
        "status",
        new MethodCall{"reply->" + type->WriteToParcelMethod(),
                       BuildVarName(*a)}});
    b->AddLiteral(kStatusOkOrBreakCheck, false /* no semicolon */);
  }

  return true;
}

}  // namespace

unique_ptr<Document> BuildServerSource(const TypeNamespace& types,
                                       const AidlInterface& parsed_doc) {
  const string bn_name = ClassName(parsed_doc, ClassNames::SERVER);
  vector<string> include_list{bn_name + ".h", kParcelHeader};
  unique_ptr<MethodImpl> on_transact{new MethodImpl{
      kAndroidStatusLiteral, bn_name, "onTransact",
      ArgList{{"uint32_t code",
               StringPrintf("const %s& data", kAndroidParcelLiteral),
               StringPrintf("%s* reply", kAndroidParcelLiteral),
               "uint32_t flags"}}
      }};

  // Declare the status variable
  on_transact->GetStatementBlock()->AddLiteral(
      StringPrintf("%s status", kAndroidStatusLiteral));

  // Add the all important switch statement, but retain a pointer to it.
  SwitchStatement* s = new SwitchStatement{"code"};
  on_transact->GetStatementBlock()->AddStatement(s);

  // The switch statement has a case statement for each transaction code.
  for (const auto& method : parsed_doc.GetMethods()) {
    StatementBlock* b = s->AddCase("Call::" + UpperCase(method->GetName()));
    if (!b) { return nullptr; }

    if (!HandleServerTransaction(types, *method, b)) { return nullptr; }
  }

  // The switch statement has a default case which defers to the super class.
  // The superclass handles a few pre-defined transactions.
  StatementBlock* b = s->AddCase("");
  b->AddLiteral(
      "status = android::BBinder::onTransact(code, data, reply, flags)");

  // Finally, the server's onTransact method just returns a status code.
  on_transact->GetStatementBlock()->AddLiteral("return status");

  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(on_transact))}};
}

unique_ptr<Document> BuildInterfaceSource(const TypeNamespace& /* types */,
                                          const AidlInterface& parsed_doc) {
  const string i_name = ClassName(parsed_doc, ClassNames::INTERFACE);
  const string bp_name = ClassName(parsed_doc, ClassNames::CLIENT);
  vector<string> include_list{i_name + ".h", bp_name + ".h"};

  string fq_name = i_name;
  if (!parsed_doc.GetPackage().empty()) {
    fq_name = StringPrintf("%s.%s", parsed_doc.GetPackage().c_str(), i_name.c_str());
  }

  unique_ptr<ConstructorDecl> meta_if{new ConstructorDecl{
      "IMPLEMENT_META_INTERFACE",
      ArgList{vector<string>{ClassName(parsed_doc, ClassNames::BASE),
                             '"' + fq_name + '"'}}}};

  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(meta_if))}};
}

unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bp_name = ClassName(interface, ClassNames::CLIENT);

  unique_ptr<ConstructorDecl> constructor{new ConstructorDecl(bp_name, {})};
  unique_ptr<ConstructorDecl> destructor{new ConstructorDecl(
      "~" + bp_name, ArgList{}, true /* is virtual */, true /* is default */)};

  vector<unique_ptr<Declaration>> publics;
  publics.push_back(std::move(constructor));
  publics.push_back(std::move(destructor));

  for (const auto& method: interface.GetMethods()) {
    publics.push_back(BuildMethodDecl(*method, types, false));
  }

  unique_ptr<ClassDecl> bp_class{
      new ClassDecl{bp_name,
                    "android::BpInterface<" + i_name + ">",
                    std::move(publics),
                    {}
      }};

  return unique_ptr<Document>{new CppHeader{
      BuildHeaderGuard(interface, ClassNames::CLIENT),
      {kIBinderHeader,
       kIInterfaceHeader,
       "utils/Errors.h",
       i_name + ".h"},
      NestInNamespaces(std::move(bp_class))}};
}

unique_ptr<Document> BuildServerHeader(const TypeNamespace& /* types */,
                                       const AidlInterface& interface) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bn_name = ClassName(interface, ClassNames::SERVER);

  unique_ptr<Declaration> on_transact{new MethodDecl{
      kAndroidStatusLiteral, "onTransact",
      ArgList{{"uint32_t code",
               StringPrintf("const %s& data", kAndroidParcelLiteral),
               StringPrintf("%s* reply", kAndroidParcelLiteral),
               "uint32_t flags = 0"}},
      MethodDecl::IS_OVERRIDE
  }};

  std::vector<unique_ptr<Declaration>> publics;
  publics.push_back(std::move(on_transact));

  unique_ptr<ClassDecl> bn_class{
      new ClassDecl{bn_name,
                    "android::BnInterface<" + i_name + ">",
                    std::move(publics),
                    {}
      }};

  return unique_ptr<Document>{new CppHeader{
      BuildHeaderGuard(interface, ClassNames::SERVER),
      {"binder/IInterface.h",
       i_name + ".h"},
      NestInNamespaces(std::move(bn_class))}};
}

unique_ptr<Document> BuildInterfaceHeader(const TypeNamespace& types,
                                          const AidlInterface& interface) {
  unique_ptr<ClassDecl> if_class{
      new ClassDecl{ClassName(interface, ClassNames::INTERFACE),
                    "android::IInterface"}};
  if_class->AddPublic(unique_ptr<Declaration>{new ConstructorDecl{
      "DECLARE_META_INTERFACE",
      ArgList{vector<string>{ClassName(interface, ClassNames::BASE)}}}});

  unique_ptr<Enum> call_enum{new Enum{"Call"}};
  for (const auto& method : interface.GetMethods()) {
    // Each method gets an enum entry and pure virtual declaration.
    if_class->AddPublic(BuildMethodDecl(*method, types, true));
    call_enum->AddValue(
        UpperCase(method->GetName()),
        StringPrintf("android::IBinder::FIRST_CALL_TRANSACTION + %d",
                     method->GetId()));
  }
  if_class->AddPublic(std::move(call_enum));

  return unique_ptr<Document>{new CppHeader{
      BuildHeaderGuard(interface, ClassNames::INTERFACE),
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
                 const AidlInterface& parsed_doc) {
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
