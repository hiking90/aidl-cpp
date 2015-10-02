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

#include "aidl_language.h"
#include "options.h"
#include "type_namespace.h"

namespace android {
namespace aidl {

int compile_aidl_to_cpp(const CppOptions& options);
int compile_aidl_to_java(const JavaOptions& options);
int preprocess_aidl(const JavaOptions& options);

namespace internals {

int load_aidl_for_test(const std::string& input_file_name,
                       const std::string& data,
                       TypeNamespace* types,
                       interface_type** returned_interface);

} // namespace internals

}  // namespace android
}  // namespace aidl

#endif  // AIDL_AIDL_H_
