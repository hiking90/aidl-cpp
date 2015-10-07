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

#include "aidl.h"
#include "aidl_language.h"
#include "ast_cpp.h"
#include "code_writer.h"
#include "tests/fake_io_delegate.h"
#include "type_cpp.h"

using android::aidl::test::FakeIoDelegate;
using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {
namespace internals {
unique_ptr<Document> BuildInterfaceSource(const TypeNamespace& types,
                                          const interface_type& parsed_doc);
unique_ptr<Document> BuildClientHeader(const TypeNamespace& types,
                                       const interface_type& parsed_doc);
unique_ptr<Document> BuildInterfaceHeader(const TypeNamespace& types,
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

namespace generated {

class BpPingResponder : public public android::BpInterface<IPingResponder> {
public:
BpPingResponder();
~BpPingResponder();
android::status_t Ping(int32_t token, int32_t* _aidl_return) override;
};  // class BpPingResponder

}  // namespace generated

}  // namespace android

#endif  // BpPingResponder_H)";

const char kExpectedTrivialInterfaceHeaderOutput[] =
R"(#include <binder/IBinder.h>
#include <binder/IInterface.h>

namespace android {

namespace generated {

class IPingResponder : public public android::IInterface {
public:
DECLARE_META_INTERFACE(PingResponder);
virtual android::status_t Ping(int32_t token, int32_t* _aidl_return) = 0;
enum Call {
  PING = android::IBinder::FIRST_CALL_TRANSACTION + 0,
}
};  // class IPingResponder

}  // namespace generated

}  // namespace android
)";

const char kExpectedTrivialInterfaceSourceOutput[] =
R"(#include <IPingResponder.h>
#include <BpPingResponder.h>

namespace android {

namespace generated {

IMPLEMENT_META_INTERFACE(PingResponder, "IPingResponder");

}  // namespace generated

}  // namespace android
)";

}  // namespace

class TrivialInterfaceASTTest : public ::testing::Test {
 protected:
  interface_type* Parse() {

  FakeIoDelegate io_delegate;
  io_delegate.SetFileContents("IPingResponder.aidl", kTrivialInterfaceAIDL);

  cpp::TypeNamespace types;
  interface_type* ret = nullptr;
  import_info* imports = nullptr;
  int err = ::android::aidl::internals::load_and_validate_aidl(
      {},  // no preprocessed files
      {},  // no import paths
      "IPingResponder.aidl",
      io_delegate,
      &types,
      &ret,
      &imports);

    if (err)
      return nullptr;

    return ret;
   }

  void Compare(Document* doc, const char* expected) {
    string output;
    unique_ptr<CodeWriter> cw = GetStringWriter(&output);

    doc->Write(cw.get());

    EXPECT_EQ(expected, output);
  }
};

TEST_F(TrivialInterfaceASTTest, GeneratesClientHeader) {
  interface_type* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientHeader(types, *interface);
  Compare(doc.get(), kExpectedTrivialClientHeaderOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesInterfaceHeader) {
  interface_type* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types, *interface);
  Compare(doc.get(), kExpectedTrivialInterfaceHeaderOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesInterfaceSource) {
  interface_type* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types, *interface);
  Compare(doc.get(), kExpectedTrivialInterfaceSourceOutput);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
