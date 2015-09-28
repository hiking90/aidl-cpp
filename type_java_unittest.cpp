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

#include "aidl_language.h"
#include "type_java.h"

using std::unique_ptr;

namespace android {
namespace aidl {
namespace {

user_data_type* MakeFakeUserDataType(const std::string& package,
                                     const std::string& class_name) {
  // This leaks memory, like all usages of these structs.
  // See b/24410295
  user_data_type* parcl = new user_data_type();
  memset(parcl, 0, sizeof(user_data_type));
  parcl->document_item.item_type = USER_DATA_TYPE;
  parcl->package = strdup(package.c_str());
  parcl->name.data = strdup(class_name.c_str());
  return parcl;
}

}  // namespace

class JavaTypeNamespaceTest : public ::testing::Test {
 protected:
  JavaTypeNamespace types_;
};

TEST_F(JavaTypeNamespaceTest, HasSomeBasicTypes) {
  EXPECT_NE(types_.Find("void"), nullptr);
  EXPECT_NE(types_.Find("int"), nullptr);
  EXPECT_NE(types_.Find("String"), nullptr);
}

TEST_F(JavaTypeNamespaceTest, ContainerTypeCreation) {
  // We start with no knowledge of parcelables or lists of them.
  EXPECT_EQ(types_.Find("Foo"), nullptr);
  EXPECT_EQ(types_.Find("List<Foo>"), nullptr);
  // Add the parcelable type we care about.
  EXPECT_TRUE(types_.AddParcelableType(MakeFakeUserDataType("a.goog", "Foo"),
                                       __FILE__));
  // Now we can find the parcelable type, but not the List of them.
  EXPECT_NE(types_.Find("Foo"), nullptr);
  EXPECT_EQ(types_.Find("List<Foo>"), nullptr);
  // But after we add the list explicitly...
  EXPECT_TRUE(types_.AddContainerType("List<Foo>"));
  // This should work.
  EXPECT_NE(types_.Find("List<Foo>"), nullptr);
}

}  // namespace android
}  // namespace aidl
