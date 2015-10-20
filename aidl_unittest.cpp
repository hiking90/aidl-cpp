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
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "aidl.h"
#include "aidl_language.h"
#include "tests/fake_io_delegate.h"
#include "type_cpp.h"
#include "type_java.h"
#include "type_namespace.h"

using android::aidl::test::FakeIoDelegate;
using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {

class AidlTest : public ::testing::Test {
 protected:
  unique_ptr<AidlInterface> Parse(const string& path,
                                  const string& contents,
                                  TypeNamespace* types) {
    FakeIoDelegate io_delegate;
    io_delegate.SetFileContents(path, contents);
    unique_ptr<AidlInterface> ret;
    std::vector<std::unique_ptr<AidlImport>> imports;
    ::android::aidl::internals::load_and_validate_aidl(
        {},  // no preprocessed files
        {},  // no import paths
        path,
        io_delegate,
        types,
        &ret,
        &imports);
    return ret;
  }
};

TEST_F(AidlTest, JavaAcceptsMissingPackage) {
  java::JavaTypeNamespace types;
  EXPECT_NE(nullptr, Parse("IFoo.aidl", "interface IFoo { }", &types));
}

TEST_F(AidlTest, CppRejectsMissingPackage) {
  cpp::TypeNamespace types;
  EXPECT_EQ(nullptr, Parse("IFoo.aidl", "interface IFoo { }", &types));
  EXPECT_NE(nullptr,
            Parse("a/IFoo.aidl", "package a; interface IFoo { }", &types));
}

TEST_F(AidlTest, RejectsOnewayOutParameters) {
  cpp::TypeNamespace cpp_types;
  java::JavaTypeNamespace java_types;
  string oneway_interface =
      "package a; oneway interface IFoo { void f(out int bar); }";
  string oneway_method =
      "package a; interface IBar { oneway void f(out int bar); }";
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_interface, &cpp_types));
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_interface, &java_types));
  EXPECT_EQ(nullptr, Parse("a/IBar.aidl", oneway_method, &cpp_types));
  EXPECT_EQ(nullptr, Parse("a/IBar.aidl", oneway_method, &java_types));
}

TEST_F(AidlTest, RejectsOnewayNonVoidReturn) {
  cpp::TypeNamespace cpp_types;
  java::JavaTypeNamespace java_types;
  string oneway_method = "package a; interface IFoo { oneway int f(); }";
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_method, &cpp_types));
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_method, &java_types));
}

TEST_F(AidlTest, AcceptsOneway) {
  cpp::TypeNamespace cpp_types;
  java::JavaTypeNamespace java_types;
  string oneway_method = "package a; interface IFoo { oneway void f(int a); }";
  string oneway_interface =
      "package a; oneway interface IBar { void f(int a); }";
  EXPECT_NE(nullptr, Parse("a/IFoo.aidl", oneway_method, &cpp_types));
  EXPECT_NE(nullptr, Parse("a/IFoo.aidl", oneway_method, &java_types));
  EXPECT_NE(nullptr, Parse("a/IBar.aidl", oneway_interface, &cpp_types));
  EXPECT_NE(nullptr, Parse("a/IBar.aidl", oneway_interface, &java_types));
}
}  // namespace aidl
}  // namespace android
