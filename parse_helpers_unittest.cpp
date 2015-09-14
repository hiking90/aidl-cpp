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

#include "parse_helpers.h"

#include <gtest/gtest.h>

namespace android {
namespace aidl {

TEST(ParseHelpersTests, RecognizesJavaKeywords) {
  EXPECT_TRUE(is_java_keyword("abstract"));
  EXPECT_TRUE(is_java_keyword("synchronized"));
  EXPECT_TRUE(is_java_keyword("volatile"));
  EXPECT_FALSE(is_java_keyword("banana"));
  EXPECT_FALSE(is_java_keyword("if in Rome"));
  EXPECT_FALSE(is_java_keyword("Default"));
}

}  // namespace android
}  // namespace aidl
