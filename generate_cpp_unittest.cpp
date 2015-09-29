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

#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "type_cpp.h"

using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {

namespace internals {
unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                       const interface_type& parsed_doc);
}

namespace {

const char kTrivialInterfaceAIDL[] =
R"(interface IPingResponder {
  int Ping(int token);
})";

const char kExpectedTrivialClientHeaderOutput[] =
R"(#ifndef BpPingResponder_H
#define BpPingResponder_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <IPingResponder.h>

namespace android {

class BpPingResponder : public public android::BpInterface<IPingResponder> {
public:
BpPingResponder();
~BpPingResponder();
virtual android::status_t Ping(int32_t token, int32_t* _aidl_return);
};  // class BpPingResponder

}  // namespace android

#endif  // BpPingResponder_H)";

}  // namespace

TEST(GenerateCPPTests, GeneratesClientHeader) {
  Parser p{"BpExampleInterface.h"};
  p.SetFileContents(kTrivialInterfaceAIDL);

  ASSERT_TRUE(p.RunParser());

  document_item_type *parsed_doc = p.GetDocument();

  ASSERT_NE(nullptr, parsed_doc);
  EXPECT_EQ(nullptr, parsed_doc->next);
  ASSERT_EQ(INTERFACE_TYPE_BINDER, parsed_doc->item_type);

  interface_type *interface = (interface_type*)parsed_doc;

  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientHeader(types, *interface);

  string output;
  unique_ptr<CodeWriter> cw = GetStringWriter(&output);

  doc->Write(cw.get());

  EXPECT_EQ(kExpectedTrivialClientHeaderOutput, output);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
