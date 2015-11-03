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
  void SetUp() override {
    java_types_.Init();
    cpp_types_.Init();
  }

  unique_ptr<AidlInterface> Parse(const string& path,
                                  const string& contents,
                                  TypeNamespace* types) {
    io_delegate_.SetFileContents(path, contents);
    unique_ptr<AidlInterface> ret;
    std::vector<std::unique_ptr<AidlImport>> imports;
    ::android::aidl::internals::load_and_validate_aidl(
        {},  // no preprocessed files
        import_paths_,
        path,
        io_delegate_,
        types,
        &ret,
        &imports);
    return ret;
  }

  FakeIoDelegate io_delegate_;
  vector<string> import_paths_;
  java::JavaTypeNamespace java_types_;
  cpp::TypeNamespace cpp_types_;
};

TEST_F(AidlTest, JavaAcceptsMissingPackage) {
  EXPECT_NE(nullptr, Parse("IFoo.aidl", "interface IFoo { }", &java_types_));
}

TEST_F(AidlTest, RejectsArraysOfBinders) {
  import_paths_.push_back("");
  io_delegate_.SetFileContents("bar/IBar.aidl",
                               "package bar; interface IBar {}");
  string path = "foo/IFoo.aidl";
  string contents = "package foo;\n"
                    "import bar.IBar;\n"
                    "interface IFoo { void f(in IBar[] input); }";
  EXPECT_EQ(nullptr, Parse(path, contents, &java_types_));
  EXPECT_EQ(nullptr, Parse(path, contents, &cpp_types_));
}

TEST_F(AidlTest, CppRejectsMissingPackage) {
  EXPECT_EQ(nullptr, Parse("IFoo.aidl", "interface IFoo { }", &cpp_types_));
  EXPECT_NE(nullptr,
            Parse("a/IFoo.aidl", "package a; interface IFoo { }", &cpp_types_));
}

TEST_F(AidlTest, RejectsOnewayOutParameters) {
  string oneway_interface =
      "package a; oneway interface IFoo { void f(out int bar); }";
  string oneway_method =
      "package a; interface IBar { oneway void f(out int bar); }";
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_interface, &cpp_types_));
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_interface, &java_types_));
  EXPECT_EQ(nullptr, Parse("a/IBar.aidl", oneway_method, &cpp_types_));
  EXPECT_EQ(nullptr, Parse("a/IBar.aidl", oneway_method, &java_types_));
}

TEST_F(AidlTest, RejectsOnewayNonVoidReturn) {
  string oneway_method = "package a; interface IFoo { oneway int f(); }";
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_method, &cpp_types_));
  EXPECT_EQ(nullptr, Parse("a/IFoo.aidl", oneway_method, &java_types_));
}

TEST_F(AidlTest, AcceptsOneway) {
  string oneway_method = "package a; interface IFoo { oneway void f(int a); }";
  string oneway_interface =
      "package a; oneway interface IBar { void f(int a); }";
  EXPECT_NE(nullptr, Parse("a/IFoo.aidl", oneway_method, &cpp_types_));
  EXPECT_NE(nullptr, Parse("a/IFoo.aidl", oneway_method, &java_types_));
  EXPECT_NE(nullptr, Parse("a/IBar.aidl", oneway_interface, &cpp_types_));
  EXPECT_NE(nullptr, Parse("a/IBar.aidl", oneway_interface, &java_types_));
}
}  // namespace aidl
}  // namespace android
