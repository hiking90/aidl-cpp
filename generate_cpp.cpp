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
#include <set>
#include <string>

#include <base/stringprintf.h>

#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "logging.h"
#include "os.h"

using android::base::StringPrintf;
using std::string;
using std::unique_ptr;
using std::vector;
using std::set;

namespace android {
namespace aidl {
namespace cpp {
namespace internals {
namespace {

const char kReturnVarName[] = "_aidl_return";
const char kAndroidStatusLiteral[] = "android::status_t";
const char kAndroidParcelLiteral[] = "android::Parcel";
const char kIBinderHeader[] = "binder/IBinder.h";
const char kIInterfaceHeader[] = "binder/IInterface.h";
const char kParcelHeader[] = "binder/Parcel.h";

unique_ptr<AstNode> BreakOnStatusNotOk() {
  IfStatement* ret = new IfStatement(new Comparison(
      new LiteralExpression("status"), "!=",
      new LiteralExpression("android::OK")));
  ret->OnTrue()->AddLiteral("break");
  return unique_ptr<AstNode>(ret);
}

unique_ptr<AstNode> ReturnOnStatusNotOk() {
  IfStatement* ret = new IfStatement(new Comparison(
      new LiteralExpression("status"), "!=",
      new LiteralExpression("android::OK")));
  ret->OnTrue()->AddLiteral("return status");
  return unique_ptr<AstNode>(ret);
}

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

      literal = type->CppType(a->GetType().IsArray());

      if (a->IsOut())
        literal += "*";
      else if (a->GetType().IsArray())
        literal = "const " + literal + "&";

      literal += " " + a->GetName();
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
          "%s* %s", return_type->CppType(false /* not array */).c_str(),
          kReturnVarName);
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

unique_ptr<CppNamespace> NestInNamespaces(
    vector<unique_ptr<Declaration>> decls,
    const vector<string>& package) {
  if (package.empty()) {
    // We should also be checking this before we get this far, but do it again
    // for the sake of unit tests and meaningful errors.
    LOG(FATAL) << "C++ generation requires a package declaration "
                  "for namespacing";
  }
  auto it = package.crbegin();  // Iterate over the namespaces inner to outer
  unique_ptr<CppNamespace> inner{new CppNamespace{*it, std::move(decls)}};
  ++it;
  for (; it != package.crend(); ++it) {
    inner.reset(new CppNamespace{*it, std::move(inner)});
  }
  return inner;
}

unique_ptr<CppNamespace> NestInNamespaces(unique_ptr<Declaration> decl,
                                          const vector<string>& package) {
  vector<unique_ptr<Declaration>> decls;
  decls.push_back(std::move(decl));
  return NestInNamespaces(std::move(decls), package);
}

bool DeclareLocalVariable(const TypeNamespace& types, const AidlArgument& a,
                          StatementBlock* b) {
  const Type* cpp_type = types.Find(a.GetType().GetName());
  if (!cpp_type) { return false; }

  string type = cpp_type->CppType(a.GetType().IsArray());

  b->AddLiteral(type + " " + BuildVarName(a));
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

string HeaderFile(const AidlInterface& interface,
                  ClassNames class_type,
                  bool use_os_sep = true) {
  string file_path = interface.GetPackage();
  for (char& c: file_path) {
    if (c == '.') {
      c = (use_os_sep) ? OS_PATH_SEPARATOR : '/';
    }
  }
  if (!file_path.empty()) {
    file_path += (use_os_sep) ? OS_PATH_SEPARATOR : '/';
  }
  file_path += ClassName(interface, class_type);
  file_path += ".h";

  return file_path;
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

  if (!interface.IsOneway() && !method.IsOneway()) {
    b->AddLiteral(StringPrintf("%s reply", kAndroidParcelLiteral));
  }

  // And declare the status variable we need for error handling.
  b->AddLiteral(StringPrintf("%s status", kAndroidStatusLiteral));

  // Serialization looks roughly like:
  //     status = data.WriteInt32(in_param_name);
  //     if (status != android::OK) { return status; }
  for (const AidlArgument* a : method.GetInArguments()) {
    string method =
      types.Find(a->GetType().GetName())
        ->WriteToParcelMethod(a->GetType().IsArray());

    string var_name = ((a->IsOut()) ? "*" : "") + a->GetName();
    b->AddStatement(new Assignment(
        "status",
        new MethodCall("data." + method, ArgList(var_name))));
    b->AddStatement(ReturnOnStatusNotOk());
  }

  // Invoke the transaction on the remote binder and confirm status.
  string transaction_code = StringPrintf(
      "%s::%s", i_name.c_str(), UpperCase(method.GetName()).c_str());

  vector<string> args = {transaction_code, "data", "&reply"};

  if (interface.IsOneway() || method.IsOneway()) {
    args.push_back("android::IBinder::FLAG_ONEWAY");
  }

  // Type checking should guarantee that nothing below emits code until "return
  // status" if we are a oneway method, so no more fear of accessing reply.

  b->AddStatement(new Assignment(
      "status",
      new MethodCall("remote()->transact",
                     ArgList(args))));
  b->AddStatement(ReturnOnStatusNotOk());

  // If the method is expected to return something, read it first by convention.
  const Type* return_type = types.Find(method.GetType().GetName());
  if (return_type != types.VoidType()) {
    string method = return_type->ReadFromParcelMethod(false);
    b->AddStatement(new Assignment(
        "status",
        new MethodCall("reply." + method, ArgList(kReturnVarName))));
    b->AddStatement(ReturnOnStatusNotOk());
  }

  for (const AidlArgument* a : method.GetOutArguments()) {
    // Deserialization looks roughly like:
    //     status = reply.ReadInt32(out_param_name);
    //     if (status != android::OK) { return status; }
    string method =
      types.Find(a->GetType().GetName())
        ->ReadFromParcelMethod(a->GetType().IsArray());

    b->AddStatement(new Assignment(
        "status",
        new MethodCall("reply." + method, ArgList(a->GetName()))));
    b->AddStatement(ReturnOnStatusNotOk());
  }

  b->AddLiteral("return status");

  return unique_ptr<Declaration>(ret.release());
}

}  // namespace

unique_ptr<Document> BuildClientSource(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  vector<string> include_list = {
      HeaderFile(interface, ClassNames::CLIENT, false),
      kParcelHeader
  };
  vector<unique_ptr<Declaration>> file_decls;

  // The constructor just passes the IBinder instance up to the super
  // class.
  file_decls.push_back(unique_ptr<Declaration>{new ConstructorImpl{
      ClassName(interface, ClassNames::CLIENT),
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
      NestInNamespaces(std::move(file_decls), interface.GetSplitPackage())}};
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
        "%s %s", return_type->CppType(false /* not array */).c_str(),
        kReturnVarName));
  }

  // Deserialize each "in" parameter to the transaction.
  for (const AidlArgument* a : method.GetInArguments()) {
    // Deserialization looks roughly like:
    //     status = data.ReadInt32(&in_param_name);
    //     if (status != android::OK) { break; }
    const Type* type = types.Find(a->GetType().GetName());
    string readMethod = type->ReadFromParcelMethod(a->GetType().IsArray());

    b->AddStatement(new Assignment{
        "status",
        new MethodCall{"data." + readMethod,
                       "&" + BuildVarName(*a)}});
    b->AddStatement(BreakOnStatusNotOk());
  }

  // Call the actual method.  This is implemented by the subclass.
  b->AddStatement(new Assignment{
      "status", new MethodCall{
          method.GetName(),
          BuildArgList(types, method, false /* not for method decl */)}});
  b->AddStatement(BreakOnStatusNotOk());

  string writeMethod =
    return_type->WriteToParcelMethod(method.GetType().IsArray());

  // If we have a return value, write it first.
  if (return_type != types.VoidType()) {
    string method = "reply->" + writeMethod;
    b->AddStatement(new Assignment{
        "status", new MethodCall{method, ArgList{kReturnVarName}}});
    b->AddStatement(BreakOnStatusNotOk());
  }

  // Write each out parameter to the reply parcel.
  for (const AidlArgument* a : method.GetOutArguments()) {
    // Serialization looks roughly like:
    //     status = data.WriteInt32(out_param_name);
    //     if (status != android::OK) { break; }
    const Type* type = types.Find(a->GetType().GetName());
    string writeMethod = type->WriteToParcelMethod(a->GetType().IsArray());

    b->AddStatement(new Assignment{
        "status",
        new MethodCall{"reply->" + writeMethod,
                       BuildVarName(*a)}});
    b->AddStatement(BreakOnStatusNotOk());
  }

  return true;
}

}  // namespace

unique_ptr<Document> BuildServerSource(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  const string bn_name = ClassName(interface, ClassNames::SERVER);
  vector<string> include_list{
      HeaderFile(interface, ClassNames::SERVER, false),
      kParcelHeader
  };
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
  for (const auto& method : interface.GetMethods()) {
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
      NestInNamespaces(std::move(on_transact), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildInterfaceSource(const TypeNamespace& /* types */,
                                          const AidlInterface& interface) {
  vector<string> include_list{
      HeaderFile(interface, ClassNames::INTERFACE, false),
      HeaderFile(interface, ClassNames::CLIENT, false),
  };

  string fq_name = ClassName(interface, ClassNames::INTERFACE);
  if (!interface.GetPackage().empty()) {
    fq_name = interface.GetPackage() + "." + fq_name;
  }

  unique_ptr<ConstructorDecl> meta_if{new ConstructorDecl{
      "IMPLEMENT_META_INTERFACE",
      ArgList{vector<string>{ClassName(interface, ClassNames::BASE),
                             '"' + fq_name + '"'}}}};

  return unique_ptr<Document>{new CppSource{
      include_list,
      NestInNamespaces(std::move(meta_if), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                       const AidlInterface& interface) {
  const string i_name = ClassName(interface, ClassNames::INTERFACE);
  const string bp_name = ClassName(interface, ClassNames::CLIENT);

  unique_ptr<ConstructorDecl> constructor{new ConstructorDecl{
      bp_name,
      ArgList{"const android::sp<android::IBinder>& impl"},
      ConstructorDecl::IS_EXPLICIT
  }};
  unique_ptr<ConstructorDecl> destructor{new ConstructorDecl{
      "~" + bp_name,
      ArgList{},
      ConstructorDecl::IS_VIRTUAL | ConstructorDecl::IS_DEFAULT}};

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
       HeaderFile(interface, ClassNames::INTERFACE, false)},
      NestInNamespaces(std::move(bp_class), interface.GetSplitPackage())}};
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
       HeaderFile(interface, ClassNames::INTERFACE, false)},
      NestInNamespaces(std::move(bn_class), interface.GetSplitPackage())}};
}

unique_ptr<Document> BuildInterfaceHeader(const TypeNamespace& types,
                                          const AidlInterface& interface) {
  set<string> includes = { kIBinderHeader, kIInterfaceHeader };

  for (const auto& method : interface.GetMethods()) {
    for (const auto& argument : method->GetArguments()) {
      const Type* type = types.Find(argument->GetType().GetName());
      const std::string& header = type->Header();
      if (! header.empty())
        includes.insert(header);
      if (argument->GetType().IsArray())
        includes.insert("vector");
    }

    const Type* type = types.Find(method->GetType().GetName());
    const std::string& header = type->Header();
    if (! header.empty())
      includes.insert(header);
  }

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
      vector<string>(includes.begin(), includes.end()),
      NestInNamespaces(std::move(if_class), interface.GetSplitPackage())}};
}

bool WriteHeader(const CppOptions& options,
                 const TypeNamespace& types,
                 const AidlInterface& interface,
                 const IoDelegate& io_delegate,
                 ClassNames header_type) {
  unique_ptr<Document> header;
  switch (header_type) {
    case ClassNames::INTERFACE:
      header = BuildInterfaceHeader(types, interface);
      break;
    case ClassNames::CLIENT:
      header = BuildClientHeader(types, interface);
      break;
    case ClassNames::SERVER:
      header = BuildServerHeader(types, interface);
      break;
    default:
      LOG(FATAL) << "aidl internal error";
  }
  if (!header) {
    LOG(ERROR) << "aidl internal error: Failed to generate header.";
    return false;
  }

  // TODO(wiley): b/25026025 error checking for file I/O
  header->Write(io_delegate.GetCodeWriter(
      options.OutputHeaderDir() + OS_PATH_SEPARATOR +
      HeaderFile(interface, header_type)).get());

  return true;
}

}  // namespace internals

using namespace internals;

bool GenerateCpp(const CppOptions& options,
                 const TypeNamespace& types,
                 const AidlInterface& interface,
                 const IoDelegate& io_delegate) {
  auto interface_src = BuildInterfaceSource(types, interface);
  auto client_src = BuildClientSource(types, interface);
  auto server_src = BuildServerSource(types, interface);

  if (!interface_src || !client_src || !server_src) {
    return false;
  }

  if (!io_delegate.CreatedNestedDirs(options.OutputHeaderDir(),
                                     interface.GetSplitPackage())) {
    LOG(ERROR) << "Failed to create directory structure for headers.";
    return false;
  }

  if (!WriteHeader(options, types, interface, io_delegate,
                   ClassNames::INTERFACE) ||
      !WriteHeader(options, types, interface, io_delegate,
                   ClassNames::CLIENT) ||
      !WriteHeader(options, types, interface, io_delegate,
                   ClassNames::SERVER)) {
    return false;
  }

  // TODO(wiley): b/25026025 error checking for file I/O.
  //              If it fails, we should remove all the partial results.
  unique_ptr<CodeWriter> writer = io_delegate.GetCodeWriter(
      options.OutputCppFilePath());
  interface_src->Write(writer.get());
  client_src->Write(writer.get());
  server_src->Write(writer.get());

  return true;
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
