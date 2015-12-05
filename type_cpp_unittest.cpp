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

#include <memory>

#include <gtest/gtest.h>

#include "type_cpp.h"

namespace android {
namespace aidl {
namespace cpp {

class CppTypeNamespaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    types_.Init();
  }
  TypeNamespace types_;
};

TEST_F(CppTypeNamespaceTest, HasSomeBasicTypes) {
  EXPECT_TRUE(types_.HasType("byte"));
  EXPECT_TRUE(types_.HasType("int"));
  EXPECT_TRUE(types_.HasType("long"));
  EXPECT_TRUE(types_.HasType("float"));
  EXPECT_TRUE(types_.HasType("double"));
  EXPECT_TRUE(types_.HasType("boolean"));
  EXPECT_TRUE(types_.HasType("char"));
  EXPECT_TRUE(types_.HasType("String"));
}

TEST_F(CppTypeNamespaceTest, SupportsListString) {
  EXPECT_TRUE(types_.HasType("List<String>"));
}

}  // namespace cpp
}  // namespace android
}  // namespace aidl
