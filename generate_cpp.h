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

#ifndef AIDL_GENERATE_CPP_H_
#define AIDL_GENERATE_CPP_H_

#include "aidl_language.h"
#include "ast_cpp.h"
#include "options.h"
#include "type_cpp.h"

namespace android {
namespace aidl {
namespace cpp {

bool GenerateCpp(const CppOptions& options,
                 const cpp::TypeNamespace& types,
                 const AidlInterface& parsed_doc);

namespace internals {
std::unique_ptr<Document> BuildClientSource(const TypeNamespace& types,
                                            const AidlInterface& parsed_doc);
std::unique_ptr<Document> BuildServerSource(const TypeNamespace& types,
                                            const AidlInterface& parsed_doc);
std::unique_ptr<Document> BuildInterfaceSource(const TypeNamespace& types,
                                               const AidlInterface& parsed_doc);
std::unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                            const AidlInterface& parsed_doc);
std::unique_ptr<Document> BuildServerHeader(const TypeNamespace& types,
                                            const AidlInterface& parsed_doc);
std::unique_ptr<Document> BuildInterfaceHeader(const TypeNamespace& types,
                                               const AidlInterface& parsed_doc);
}
}  // namespace cpp
}  // namespace aidl
}  // namespace android

#endif // AIDL_GENERATE_CPP_H_
