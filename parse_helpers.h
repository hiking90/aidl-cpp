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

#ifndef AIDL_PARSE_HELPERS_H_
#define AIDL_PARSE_HELPERS_H_

#include <string>

#include "aidl_language.h"

namespace android {
namespace aidl {

bool is_java_keyword(const char* str);

char* cpp_strdup(const char* in);

std::string gather_comments(extra_text_type* extra);

}  // namespace android
}  // namespace aidl

#endif // AIDL_PARSE_HELPERS_H_
