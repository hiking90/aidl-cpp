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

#ifndef AIDL_AIDL_H_
#define AIDL_AIDL_H_

#include "options.h"

namespace android {
namespace aidl {

// strips off the leading whitespace, the "import" text
// also returns whether it's a local or system import
// we rely on the input matching the import regex from below
char* parse_import_statement(const char* text);

int convert_direction(const char* direction);

int compile_aidl(const JavaOptions& options);
int preprocess_aidl(const JavaOptions& options);

}  // namespace android
}  // namespace aidl

#endif  // AIDL_AIDL_H_
