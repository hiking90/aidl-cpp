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

#include <string>

#include <gtest/gtest.h>

#include "ast_cpp.h"
#include "code_writer.h"

using std::string;
using std::vector;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {
namespace {

const char kExpectedHeaderOutput[] =
R"(#ifndef HEADER_INCLUDE_GUARD_H_
#define HEADER_INCLUDE_GUARD_H_

#include <string>
#include <memory>

namespace android {

namespace test {

class TestClass {
public:
void NormalMethod(int normalarg, float normal2);
virtual void SubMethod(int subarg) const;
};  // class TestClass

class TestSubClass : public TestClass {
public:
virtual void SubMethod(int subarg) const;
};  // class TestSubClass

}  // namespace test

}  // namespace android

#endif  // HEADER_INCLUDE_GUARD_H_)";

const char kExpectedEnumOutput[] =
R"(enum Foo {
  BAR = 42,
  BAZ,
}
)";

}  // namespace

TEST(AstCppTests, GeneratesHeader) {
  unique_ptr<MethodDecl> norm{
      new MethodDecl("void", "NormalMethod",
                     { "int normalarg", "float normal2" })};
  unique_ptr<MethodDecl> sub{
      new MethodDecl("void", "SubMethod",
                     { "int subarg" },
                     MethodDecl::IS_CONST | MethodDecl::IS_VIRTUAL)};
  unique_ptr<MethodDecl> sub2{
      new MethodDecl("void", "SubMethod",
                     { "int subarg" },
                     MethodDecl::IS_CONST | MethodDecl::IS_VIRTUAL)};
  vector<unique_ptr<Declaration>> test_methods;
  test_methods.push_back(std::move(norm));
  test_methods.push_back(std::move(sub));

  vector<unique_ptr<Declaration>> test_sub_methods;
  test_sub_methods.push_back(std::move(sub2));

  unique_ptr<Declaration> test{new ClassDecl { "TestClass", "",
      std::move(test_methods), {} }};

  unique_ptr<Declaration> test_sub{new ClassDecl { "TestSubClass",
      "TestClass", std::move(test_sub_methods), {} }};

  vector<unique_ptr<Declaration>> classes;
  classes.push_back(std::move(test));
  classes.push_back(std::move(test_sub));

  unique_ptr<CppNamespace> test_ns{new CppNamespace {"test",
      std::move(classes)}};

  vector<unique_ptr<Declaration>> test_ns_vec;
  test_ns_vec.push_back(std::move(test_ns));

  unique_ptr<CppNamespace> android_ns{new CppNamespace {"android", std::move(test_ns_vec) }};

  CppHeader cpp_header{"HEADER_INCLUDE_GUARD_H_", {"string", "memory"},
      std::move(android_ns) };
  string actual_output;
  CodeWriterPtr writer = GetStringWriter(&actual_output);
  cpp_header.Write(writer.get());
  EXPECT_EQ(string(kExpectedHeaderOutput), actual_output);
}

TEST(AstCppTests, GeneratesEnum) {
  Enum e("Foo");
  e.AddValue("BAR", "42");
  e.AddValue("BAZ", "");
  string actual_output;
  CodeWriterPtr writer = GetStringWriter(&actual_output);
  e.Write(writer.get());
  EXPECT_EQ(string(kExpectedEnumOutput), actual_output);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
