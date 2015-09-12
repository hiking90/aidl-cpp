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

}  // namespace test
}  // namespace android

#endif  // HEADER_INCLUDE_GUARD_H_)";

}  // namespace

TEST(AstCppTests, GeneratesHeader) {
  CppHeader cpp_header("HEADER_INCLUDE_GUARD_H_",
                       {"string", "memory"},
                       {"android", "test"});
  string actual_output;
  CodeWriterPtr writer = get_string_writer(&actual_output);
  cpp_header.Write(writer.get());
  EXPECT_EQ(string(kExpectedHeaderOutput), actual_output);
}

}  // namespace aidl
}  // namespace android
