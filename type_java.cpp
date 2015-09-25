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

#include "type_java.h"

#include <sys/types.h>

#include <base/strings.h>

#include "aidl_language.h"
#include "logging.h"

using android::base::Split;
using android::base::Join;
using android::base::Trim;

namespace android {
namespace aidl {

Expression* NULL_VALUE;
Expression* THIS_VALUE;
Expression* SUPER_VALUE;
Expression* TRUE_VALUE;
Expression* FALSE_VALUE;

// ================================================================

Type::Type(const JavaTypeNamespace* types, const string& name, int kind,
           bool canWriteToParcel, bool canBeOut)
    : m_types(types),
      m_package(),
      m_name(name),
      m_declFile(""),
      m_declLine(-1),
      m_kind(kind),
      m_canWriteToParcel(canWriteToParcel),
      m_canBeOut(canBeOut) {
  m_qualifiedName = name;
}

Type::Type(const JavaTypeNamespace* types, const string& package,
           const string& name, int kind, bool canWriteToParcel, bool canBeOut,
           const string& declFile, int declLine)
    : m_types(types),
      m_package(package),
      m_name(name),
      m_declFile(declFile),
      m_declLine(declLine),
      m_kind(kind),
      m_canWriteToParcel(canWriteToParcel),
      m_canBeOut(canBeOut) {
  if (package.length() > 0) {
    m_qualifiedName = package;
    m_qualifiedName += '.';
  }
  m_qualifiedName += name;
}

Type::~Type() {}

string Type::HumanReadableKind() const {
  switch (Kind()) {
    case INTERFACE:
      return "an interface";
    case USERDATA:
      return "a user data";
    default:
      return "ERROR";
  }
}

bool Type::CanBeArray() const { return false; }

string Type::ImportType() const { return m_qualifiedName; }

string Type::CreatorName() const { return ""; }

string Type::InstantiableName() const { return QualifiedName(); }

void Type::WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                         int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%sn", __FILE__,
          __LINE__, m_qualifiedName.c_str());
  addTo->Add(new LiteralExpression("/* WriteToParcel error " + m_qualifiedName +
                                   " */"));
}

void Type::CreateFromParcel(StatementBlock* addTo, Variable* v,
                            Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_qualifiedName.c_str());
  addTo->Add(new LiteralExpression("/* CreateFromParcel error " +
                                   m_qualifiedName + " */"));
}

void Type::ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                          Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_qualifiedName.c_str());
  addTo->Add(new LiteralExpression("/* ReadFromParcel error " +
                                   m_qualifiedName + " */"));
}

void Type::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                              Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_qualifiedName.c_str());
  addTo->Add(new LiteralExpression("/* WriteArrayToParcel error " +
                                   m_qualifiedName + " */"));
}

void Type::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                 Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_qualifiedName.c_str());
  addTo->Add(new LiteralExpression("/* CreateArrayFromParcel error " +
                                   m_qualifiedName + " */"));
}

void Type::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_qualifiedName.c_str());
  addTo->Add(new LiteralExpression("/* ReadArrayFromParcel error " +
                                   m_qualifiedName + " */"));
}

void Type::SetQualifiedName(const string& qualified) {
  m_qualifiedName = qualified;
}

Expression* Type::BuildWriteToParcelFlags(int flags) const {
  if (flags == 0) {
    return new LiteralExpression("0");
  }
  if ((flags & PARCELABLE_WRITE_RETURN_VALUE) != 0) {
    return new FieldVariable(m_types->ParcelableInterfaceType(),
                             "PARCELABLE_WRITE_RETURN_VALUE");
  }
  return new LiteralExpression("0");
}

// ================================================================

BasicType::BasicType(const JavaTypeNamespace* types, const string& name,
                     const string& marshallParcel,
                     const string& unmarshallParcel,
                     const string& writeArrayParcel,
                     const string& createArrayParcel,
                     const string& readArrayParcel)
    : Type(types, name, BUILT_IN, true, false),
      m_marshallParcel(marshallParcel),
      m_unmarshallParcel(unmarshallParcel),
      m_writeArrayParcel(writeArrayParcel),
      m_createArrayParcel(createArrayParcel),
      m_readArrayParcel(readArrayParcel) {}

void BasicType::WriteToParcel(StatementBlock* addTo, Variable* v,
                              Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, m_marshallParcel, 1, v));
}

void BasicType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                 Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, m_unmarshallParcel)));
}

bool BasicType::CanBeArray() const { return true; }

void BasicType::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, m_writeArrayParcel, 1, v));
}

void BasicType::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, m_createArrayParcel)));
}

void BasicType::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, m_readArrayParcel, 1, v));
}

// ================================================================

BooleanType::BooleanType(const JavaTypeNamespace* types)
    : Type(types, "boolean", BUILT_IN, true, false) {}

void BooleanType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(
      parcel, "writeInt", 1,
      new Ternary(v, new LiteralExpression("1"), new LiteralExpression("0"))));
}

void BooleanType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  addTo->Add(
      new Assignment(v, new Comparison(new LiteralExpression("0"), "!=",
                                       new MethodCall(parcel, "readInt"))));
}

bool BooleanType::CanBeArray() const { return true; }

void BooleanType::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeBooleanArray", 1, v));
}

void BooleanType::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createBooleanArray")));
}

void BooleanType::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readBooleanArray", 1, v));
}

// ================================================================

CharType::CharType(const JavaTypeNamespace* types)
    : Type(types, "char", BUILT_IN, true, false) {}

void CharType::WriteToParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, int flags) const {
  addTo->Add(
      new MethodCall(parcel, "writeInt", 1, new Cast(m_types->IntType(), v)));
}

void CharType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readInt"), this));
}

bool CharType::CanBeArray() const { return true; }

void CharType::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeCharArray", 1, v));
}

void CharType::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createCharArray")));
}

void CharType::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readCharArray", 1, v));
}

// ================================================================

StringType::StringType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "String", BUILT_IN, true, false) {}

string StringType::CreatorName() const {
  return "android.os.Parcel.STRING_CREATOR";
}

void StringType::WriteToParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeString", 1, v));
}

void StringType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readString")));
}

bool StringType::CanBeArray() const { return true; }

void StringType::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeStringArray", 1, v));
}

void StringType::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createStringArray")));
}

void StringType::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readStringArray", 1, v));
}

// ================================================================

CharSequenceType::CharSequenceType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "CharSequence", BUILT_IN, true, false) {}

string CharSequenceType::CreatorName() const {
  return "android.os.Parcel.STRING_CREATOR";
}

void CharSequenceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, int flags) const {
  // if (v != null) {
  //     parcel.writeInt(1);
  //     v.writeToParcel(parcel);
  // } else {
  //     parcel.writeInt(0);
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("0")));
  IfStatement* ifpart = new IfStatement;
  ifpart->expression = new Comparison(v, "!=", NULL_VALUE);
  ifpart->elseif = elsepart;
  ifpart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("1")));
  ifpart->statements->Add(new MethodCall(m_types->TextUtilsType(),
                                         "writeToParcel", 3, v, parcel,
                                         BuildWriteToParcelFlags(flags)));

  addTo->Add(ifpart);
}

void CharSequenceType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, Variable**) const {
  // if (0 != parcel.readInt()) {
  //     v = TextUtils.createFromParcel(parcel)
  // } else {
  //     v = null;
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(new Assignment(v, NULL_VALUE));

  IfStatement* ifpart = new IfStatement();
  ifpart->expression = new Comparison(new LiteralExpression("0"), "!=",
                                      new MethodCall(parcel, "readInt"));
  ifpart->elseif = elsepart;
  ifpart->statements->Add(new Assignment(
      v, new MethodCall(m_types->TextUtilsType(),
                        "CHAR_SEQUENCE_CREATOR.createFromParcel", 1, parcel)));

  addTo->Add(ifpart);
}

// ================================================================

RemoteExceptionType::RemoteExceptionType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "RemoteException", BUILT_IN, false, false) {}

void RemoteExceptionType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void RemoteExceptionType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                           Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

RuntimeExceptionType::RuntimeExceptionType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "RuntimeException", BUILT_IN, false, false) {}

void RuntimeExceptionType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                         Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void RuntimeExceptionType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                            Variable* parcel,
                                            Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

IBinderType::IBinderType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "IBinder", BUILT_IN, true, false) {}

void IBinderType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeStrongBinder", 1, v));
}

void IBinderType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readStrongBinder")));
}

void IBinderType::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeBinderArray", 1, v));
}

void IBinderType::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createBinderArray")));
}

void IBinderType::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readBinderArray", 1, v));
}

// ================================================================

IInterfaceType::IInterfaceType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "IInterface", BUILT_IN, false, false) {}

void IInterfaceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void IInterfaceType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

BinderType::BinderType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "Binder", BUILT_IN, false, false) {}

void BinderType::WriteToParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void BinderType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

BinderProxyType::BinderProxyType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "BinderProxy", BUILT_IN, false, false) {}

void BinderProxyType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void BinderProxyType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

ParcelType::ParcelType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "Parcel", BUILT_IN, false, false) {}

void ParcelType::WriteToParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void ParcelType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

ParcelableInterfaceType::ParcelableInterfaceType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "Parcelable", BUILT_IN, false, false) {}

void ParcelableInterfaceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                            Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void ParcelableInterfaceType::CreateFromParcel(StatementBlock* addTo,
                                               Variable* v, Variable* parcel,
                                               Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

MapType::MapType(const JavaTypeNamespace* types)
    : Type(types, "java.util", "Map", BUILT_IN, true, true) {}

void MapType::WriteToParcel(StatementBlock* addTo, Variable* v,
                            Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeMap", 1, v));
}

static void EnsureClassLoader(StatementBlock* addTo, Variable** cl,
                              const JavaTypeNamespace* types) {
  // We don't want to look up the class loader once for every
  // collection argument, so ensure we do it at most once per method.
  if (*cl == NULL) {
    *cl = new Variable(types->ClassLoaderType(), "cl");
    addTo->Add(new VariableDeclaration(
        *cl, new LiteralExpression("this.getClass().getClassLoader()"),
        types->ClassLoaderType()));
  }
}

void MapType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readHashMap", 1, *cl)));
}

void MapType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(new MethodCall(parcel, "readMap", 2, v, *cl));
}

// ================================================================

ListType::ListType(const JavaTypeNamespace* types)
    : Type(types, "java.util", "List", BUILT_IN, true, true) {}

string ListType::InstantiableName() const { return "java.util.ArrayList"; }

void ListType::WriteToParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeList", 1, v));
}

void ListType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(
      new Assignment(v, new MethodCall(parcel, "readArrayList", 1, *cl)));
}

void ListType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                              Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(new MethodCall(parcel, "readList", 2, v, *cl));
}

// ================================================================

UserDataType::UserDataType(const JavaTypeNamespace* types,
                           const string& package, const string& name,
                           bool builtIn, bool canWriteToParcel,
                           const string& declFile, int declLine)
    : Type(types, package, name, builtIn ? BUILT_IN : USERDATA,
           canWriteToParcel, true, declFile, declLine) {}

string UserDataType::CreatorName() const {
  return QualifiedName() + ".CREATOR";
}

void UserDataType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                 Variable* parcel, int flags) const {
  // if (v != null) {
  //     parcel.writeInt(1);
  //     v.writeToParcel(parcel);
  // } else {
  //     parcel.writeInt(0);
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("0")));
  IfStatement* ifpart = new IfStatement;
  ifpart->expression = new Comparison(v, "!=", NULL_VALUE);
  ifpart->elseif = elsepart;
  ifpart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("1")));
  ifpart->statements->Add(new MethodCall(v, "writeToParcel", 2, parcel,
                                         BuildWriteToParcelFlags(flags)));

  addTo->Add(ifpart);
}

void UserDataType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, Variable**) const {
  // if (0 != parcel.readInt()) {
  //     v = CLASS.CREATOR.createFromParcel(parcel)
  // } else {
  //     v = null;
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(new Assignment(v, NULL_VALUE));

  IfStatement* ifpart = new IfStatement();
  ifpart->expression = new Comparison(new LiteralExpression("0"), "!=",
                                      new MethodCall(parcel, "readInt"));
  ifpart->elseif = elsepart;
  ifpart->statements->Add(new Assignment(
      v, new MethodCall(v->type, "CREATOR.createFromParcel", 1, parcel)));

  addTo->Add(ifpart);
}

void UserDataType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  // TODO: really, we don't need to have this extra check, but we
  // don't have two separate marshalling code paths
  // if (0 != parcel.readInt()) {
  //     v.readFromParcel(parcel)
  // }
  IfStatement* ifpart = new IfStatement();
  ifpart->expression = new Comparison(new LiteralExpression("0"), "!=",
                                      new MethodCall(parcel, "readInt"));
  ifpart->statements->Add(new MethodCall(v, "readFromParcel", 1, parcel));
  addTo->Add(ifpart);
}

bool UserDataType::CanBeArray() const { return true; }

void UserDataType::WriteArrayToParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeTypedArray", 2, v,
                            BuildWriteToParcelFlags(flags)));
}

void UserDataType::CreateArrayFromParcel(StatementBlock* addTo, Variable* v,
                                         Variable* parcel, Variable**) const {
  string creator = v->type->QualifiedName() + ".CREATOR";
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createTypedArray", 1,
                                              new LiteralExpression(creator))));
}

void UserDataType::ReadArrayFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  string creator = v->type->QualifiedName() + ".CREATOR";
  addTo->Add(new MethodCall(parcel, "readTypedArray", 2, v,
                            new LiteralExpression(creator)));
}

// ================================================================

InterfaceType::InterfaceType(const JavaTypeNamespace* types,
                             const string& package, const string& name,
                             bool builtIn, bool oneway, const string& declFile,
                             int declLine)
    : Type(types, package, name, builtIn ? BUILT_IN : INTERFACE, true, false,
           declFile, declLine),
      m_oneway(oneway) {}

bool InterfaceType::OneWay() const { return m_oneway; }

void InterfaceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, int flags) const {
  // parcel.writeStrongBinder(v != null ? v.asBinder() : null);
  addTo->Add(
      new MethodCall(parcel, "writeStrongBinder", 1,
                     new Ternary(new Comparison(v, "!=", NULL_VALUE),
                                 new MethodCall(v, "asBinder"), NULL_VALUE)));
}

void InterfaceType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  // v = Interface.asInterface(parcel.readStrongBinder());
  string stub_type = v->type->QualifiedName() + ".Stub";
  addTo->Add(new Assignment(
      v, new MethodCall(m_types->Find(stub_type), "asInterface", 1,
                        new MethodCall(parcel, "readStrongBinder"))));
}

// ================================================================

GenericType::GenericType(const JavaTypeNamespace* types, const string& package,
                         const string& name, const vector<const Type*>& args)
    : Type(types, package, name, BUILT_IN, true, true) {
  m_args = args;

  m_importName = package + '.' + name;

  string gen = "<";
  int N = args.size();
  for (int i = 0; i < N; i++) {
    const Type* t = args[i];
    gen += t->QualifiedName();
    if (i != N - 1) {
      gen += ',';
    }
  }
  gen += '>';
  m_genericArguments = gen;
  SetQualifiedName(m_importName + gen);
}

const vector<const Type*>& GenericType::GenericArgumentTypes() const {
  return m_args;
}

string GenericType::GenericArguments() const { return m_genericArguments; }

string GenericType::ImportType() const { return m_importName; }

void GenericType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, int flags) const {
  fprintf(stderr, "implement GenericType::WriteToParcel\n");
}

void GenericType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  fprintf(stderr, "implement GenericType::CreateFromParcel\n");
}

void GenericType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                 Variable* parcel, Variable**) const {
  fprintf(stderr, "implement GenericType::ReadFromParcel\n");
}

// ================================================================

GenericListType::GenericListType(const JavaTypeNamespace* types,
                                 const string& package, const string& name,
                                 const vector<const Type*>& args)
    : GenericType(types, package, name, args),
      m_creator(args[0]->CreatorName()) {}

string GenericListType::CreatorName() const {
  return "android.os.Parcel.arrayListCreator";
}

string GenericListType::InstantiableName() const {
  return "java.util.ArrayList" + GenericArguments();
}

void GenericListType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, int flags) const {
  if (m_creator == m_types->StringType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "writeStringList", 1, v));
  } else if (m_creator == m_types->IBinderType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "writeBinderList", 1, v));
  } else {
    // parcel.writeTypedListXX(arg);
    addTo->Add(new MethodCall(parcel, "writeTypedList", 1, v));
  }
}

void GenericListType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  if (m_creator == m_types->StringType()->CreatorName()) {
    addTo->Add(
        new Assignment(v, new MethodCall(parcel, "createStringArrayList", 0)));
  } else if (m_creator == m_types->IBinderType()->CreatorName()) {
    addTo->Add(
        new Assignment(v, new MethodCall(parcel, "createBinderArrayList", 0)));
  } else {
    // v = _data.readTypedArrayList(XXX.creator);
    addTo->Add(
        new Assignment(v, new MethodCall(parcel, "createTypedArrayList", 1,
                                         new LiteralExpression(m_creator))));
  }
}

void GenericListType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  if (m_creator == m_types->StringType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "readStringList", 1, v));
  } else if (m_creator == m_types->IBinderType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "readBinderList", 1, v));
  } else {
    // v = _data.readTypedList(v, XXX.creator);
    addTo->Add(new MethodCall(parcel, "readTypedList", 2, v,
                              new LiteralExpression(m_creator)));
  }
}

// ================================================================

ClassLoaderType::ClassLoaderType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "ClassLoader", BUILT_IN, false, false) {}

// ================================================================

JavaTypeNamespace::ContainerClass::ContainerClass(const string& package,
                                                  const string& class_name,
                                                  size_t nargs)
    : package(package),
      class_name(class_name),
      canonical_name(package + "." + class_name),
      args(nargs) {
}

JavaTypeNamespace::JavaTypeNamespace() {
  Add(new BasicType(this, "void", "XXX", "XXX", "XXX", "XXX", "XXX"));

  m_bool_type = new BooleanType(this);
  Add(m_bool_type);

  Add(new BasicType(this, "byte", "writeByte", "readByte", "writeByteArray",
                    "createByteArray", "readByteArray"));

  Add(new CharType(this));

  m_int_type = new BasicType(this, "int", "writeInt", "readInt",
                             "writeIntArray", "createIntArray", "readIntArray");
  Add(m_int_type);

  Add(new BasicType(this, "long", "writeLong", "readLong", "writeLongArray",
                    "createLongArray", "readLongArray"));

  Add(new BasicType(this, "float", "writeFloat", "readFloat", "writeFloatArray",
                    "createFloatArray", "readFloatArray"));

  Add(new BasicType(this, "double", "writeDouble", "readDouble",
                    "writeDoubleArray", "createDoubleArray",
                    "readDoubleArray"));

  m_string_type = new class StringType(this);
  Add(m_string_type);

  Add(new Type(this, "java.lang", "Object", Type::BUILT_IN, false, false));

  Add(new CharSequenceType(this));

  Add(new MapType(this));

  Add(new ListType(this));

  m_text_utils_type =
      new Type(this, "android.text", "TextUtils", Type::BUILT_IN, false, false);
  Add(m_text_utils_type);

  m_remote_exception_type = new class RemoteExceptionType(this);
  Add(m_remote_exception_type);

  m_runtime_exception_type = new class RuntimeExceptionType(this);
  Add(m_runtime_exception_type);

  m_ibinder_type = new class IBinderType(this);
  Add(m_ibinder_type);

  m_iinterface_type = new class IInterfaceType(this);
  Add(m_iinterface_type);

  m_binder_native_type = new class BinderType(this);
  Add(m_binder_native_type);

  m_binder_proxy_type = new class BinderProxyType(this);
  Add(m_binder_proxy_type);

  m_parcel_type = new class ParcelType(this);
  Add(m_parcel_type);

  m_parcelable_interface_type = new class ParcelableInterfaceType(this);
  Add(m_parcelable_interface_type);

  m_context_type = new class Type(this, "android.content", "Context",
                                  Type::BUILT_IN, false, false);
  Add(m_context_type);

  m_classloader_type = new class ClassLoaderType(this);
  Add(m_classloader_type);

  NULL_VALUE = new LiteralExpression("null");
  THIS_VALUE = new LiteralExpression("this");
  SUPER_VALUE = new LiteralExpression("super");
  TRUE_VALUE = new LiteralExpression("true");
  FALSE_VALUE = new LiteralExpression("false");

  m_containers.emplace_back("java.util", "List", 1);
  m_containers.emplace_back("java.util", "Map", 2);
}

JavaTypeNamespace::~JavaTypeNamespace() {
  int N = m_types.size();
  for (int i = 0; i < N; i++) {
    delete m_types[i];
  }
}

bool JavaTypeNamespace::Add(const Type* type) {
  const Type* existing = Find(type->QualifiedName());
  if (!existing) {
    m_types.push_back(type);
    return true;
  }

  if (existing->Kind() == Type::BUILT_IN) {
    fprintf(stderr, "%s:%d attempt to redefine built in class %s\n",
            type->DeclFile().c_str(), type->DeclLine(),
            type->QualifiedName().c_str());
    return false;
  }

  if (type->Kind() != existing->Kind()) {
    fprintf(stderr, "%s:%d attempt to redefine %s as %s,\n",
            type->DeclFile().c_str(), type->DeclLine(),
            type->QualifiedName().c_str(), type->HumanReadableKind().c_str());
    fprintf(stderr, "%s:%d previously defined here as %s.\n",
            existing->DeclFile().c_str(), existing->DeclLine(),
            existing->HumanReadableKind().c_str());
    return false;
  }

  return true;
}

const Type* JavaTypeNamespace::Find(const string& unstripped_name) const {
  const ContainerClass* g = nullptr;
  vector<const Type*> template_arg_types;
  if (!CanonicalizeContainerClass(unstripped_name, &g, &template_arg_types)) {
    LOG(ERROR) << "Error canonicalizing type '" << unstripped_name << "'";
    return nullptr;
  }

  string name = Trim(unstripped_name);
  if (g != nullptr) {
    vector<string> template_args;
    for (const Type* type : template_arg_types) {
      template_args.push_back(type->QualifiedName());
     }
    name = g->canonical_name + "<" + Join(template_args, ',') + ">";
   }

  // Always prefer a exact match if possible.
  // This works for primitives and class names qualified with a package.
  for (const Type* type : m_types) {
    if (type->QualifiedName() == name) {
      return type;
    }
  }

  // We allow authors to drop packages when refering to a class name.
  // our language doesn't allow you to not specify outer classes
  // when referencing an inner class.  that could be changed, and this
  // would be the place to do it, but I don't think the complexity in
  // scoping rules is worth it.
  for (const Type* type : m_types) {
    if (type->Name() == name) {
      return type;
    }
  }

  return nullptr;
}

const Type* JavaTypeNamespace::Find(const char* package,
                                    const char* name) const {
  string s;
  if (package != nullptr && *package != '\0') {
    s += package;
    s += '.';
  }
  s += name;
  return Find(s);
}

bool JavaTypeNamespace::AddParcelableType(const user_data_type* p,
                                          const std::string& filename) {
  Type* type =
      new UserDataType(this, p->package ? p->package : "", p->name.data, false,
                       p->parcelable, filename, p->name.lineno);
  return Add(type);
}

bool JavaTypeNamespace::AddBinderType(const interface_type* b,
                                      const std::string& filename) {
  // for interfaces, add the stub, proxy, and interface types.
  Type* type =
      new InterfaceType(this, b->package ? b->package : "", b->name.data, false,
                        b->oneway, filename, b->name.lineno);
  Type* stub = new Type(this, b->package ? b->package : "",
                        string{b->name.data} + ".Stub", Type::GENERATED, false,
                        false, filename, b->name.lineno);
  Type* proxy = new Type(this, b->package ? b->package : "",
                         string{b->name.data} + ".Stub.Proxy", Type::GENERATED,
                         false, false, filename, b->name.lineno);

  bool success = true;
  success &= Add(type);
  success &= Add(stub);
  success &= Add(proxy);
  return success;
}

bool JavaTypeNamespace::AddContainerType(const string& type_name) {
  if (Find(type_name) != nullptr) {
    return true;  // Don't add duplicates of the same templated type.
  }

  const ContainerClass* g = nullptr;
  vector<const Type*> template_arg_types;
  if (!CanonicalizeContainerClass(type_name, &g, &template_arg_types)) {
    LOG(ERROR) << "Error canonicalizing type '" << type_name << "'";
    return false;
  }

  if (g == nullptr) {
    // We parsed the type correctly, but it wasn't a container type.  No error.
    return true;
  }

  // construct an instance of a container type, add it to our name set so they
  // always get the same object, and return it.
  Type* result = nullptr;
  if (g->canonical_name == "java.util.List" &&
      template_arg_types.size() == 1u) {
    result = new GenericListType(this, g->package, g->class_name,
                                 template_arg_types);
  } else {
    LOG(ERROR) << "Don't know how to create a container of type "
               << g->canonical_name << " with " << template_arg_types.size()
               << " arguments.";
    return false;
  }

  Add(result);
  return true;
}

const ValidatableType* JavaTypeNamespace::GetValidatableType(
    const string& name) const {
  return Find(name);
}

const JavaTypeNamespace::ContainerClass* JavaTypeNamespace::FindContainerClass(
    const string& name,
    size_t nargs) const {
  // first check fully qualified class names (with packages).
  for (const ContainerClass& container : m_containers) {
    if (container.canonical_name == name && nargs == container.args) {
      return &container;
    }
  }

  // then match on the class name alone (no package).
  for (const ContainerClass& container : m_containers) {
    if (container.class_name == name && nargs == container.args) {
      return &container;
    }
  }

  return nullptr;
}

bool JavaTypeNamespace::CanonicalizeContainerClass(
    const string& raw_name,
    const ContainerClass** container_class,
    vector<const Type*>* arg_types) const {
  string name = Trim(raw_name);
  const size_t opening_brace = name.find('<');
  if (opening_brace == name.npos) {
    // Not a template type and not an error.
    *container_class = nullptr;
    arg_types->clear();
    return true;
  }

  const size_t closing_brace = name.find('>');
  if (opening_brace != name.rfind('<') ||
      closing_brace != name.rfind('>') ||
      closing_brace != name.length() - 1) {
    LOG(ERROR) << "Invalid template type '" << name << "'";
    // Nested/invalid templates are forbidden.
    return false;
  }

  string container_class_name = Trim(name.substr(0, opening_brace));
  string remainder = name.substr(opening_brace + 1,
                                 (closing_brace - opening_brace) - 1);
  vector<string> template_args = Split(remainder, ",");

  const ContainerClass* g =
      FindContainerClass(container_class_name, template_args.size());
  if (g == nullptr) {
    LOG(ERROR) << "Failed to find templated container '"
               << container_class_name << "'";
    return false;
  }

  vector<const Type*> template_arg_types;
  for (auto& template_arg : template_args) {
    // Recursively search for the contained types.
    const Type* template_arg_type = Find(Trim(template_arg));
    if (template_arg_type == nullptr) {
      LOG(ERROR) << "Failed to find formal type of '"
                 << template_arg << "'";
      return false;
    }
    template_arg_types.push_back(template_arg_type);
  }

  *arg_types = template_arg_types;
  *container_class = g;
  return true;
}

void JavaTypeNamespace::Dump() const {
  int n = m_types.size();
  for (int i = 0; i < n; i++) {
    const Type* t = m_types[i];
    printf("type: package=%s name=%s qualifiedName=%s\n", t->Package().c_str(),
           t->Name().c_str(), t->QualifiedName().c_str());
  }
}

}  // namespace aidl
}  // namespace android
