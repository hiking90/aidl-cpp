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

}  // namespace

TEST(AstCppTests, GeneratesHeader) {
  unique_ptr<CppMethodDeclaration> norm{new CppMethodDeclaration("void",
                                                                "NormalMethod",
                                                                { "int normalarg",
                                                                "float normal2" })};
  unique_ptr<CppMethodDeclaration> sub{new CppMethodDeclaration("void",
                                                                "SubMethod",
                                                                { "int subarg" },
                                                                true,
                                                                true)};
  unique_ptr<CppMethodDeclaration> sub2{new CppMethodDeclaration("void",
                                                                 "SubMethod",
                                                                 { "int subarg" },
                                                                 true,
                                                                 true)};
  vector<unique_ptr<CppDeclaration>> test_methods;
  test_methods.push_back(std::move(norm));
  test_methods.push_back(std::move(sub));

  vector<unique_ptr<CppDeclaration>> test_sub_methods;
  test_sub_methods.push_back(std::move(sub2));

  unique_ptr<CppDeclaration> test{new CppClassDeclaration { "TestClass", "",
      std::move(test_methods), {} }};

  unique_ptr<CppDeclaration> test_sub{new CppClassDeclaration { "TestSubClass",
      "TestClass", std::move(test_sub_methods), {} }};

  vector<unique_ptr<CppDeclaration>> classes;
  classes.push_back(std::move(test));
  classes.push_back(std::move(test_sub));

  unique_ptr<CppNamespace> test_ns{new CppNamespace {"test",
      std::move(classes)}};

  vector<unique_ptr<CppDeclaration>> test_ns_vec;
  test_ns_vec.push_back(std::move(test_ns));

  unique_ptr<CppNamespace> android_ns{new CppNamespace {"android", std::move(test_ns_vec) }};

  CppHeader cpp_header{"HEADER_INCLUDE_GUARD_H_", {"string", "memory"},
      std::move(android_ns) };
  string actual_output;
  CodeWriterPtr writer = GetStringWriter(&actual_output);
  cpp_header.Write(writer.get());
  EXPECT_EQ(string(kExpectedHeaderOutput), actual_output);
}

}  // namespace aidl
}  // namespace android
