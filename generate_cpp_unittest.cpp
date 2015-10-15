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
#include "generate_cpp.h"
#include "tests/fake_io_delegate.h"
#include "type_cpp.h"

using android::aidl::test::FakeIoDelegate;
using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {
namespace {

const char kTrivialInterfaceAIDL[] =
R"(interface IPingResponder {
  int Ping(int token);
})";

const char kExpectedTrivialClientSourceOutput[] =
R"(#include <BpPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace generated {

BpPingResponder::BpPingResponder(const android::sp<android::IBinder>& impl)
    : BpInterface<IPingResponder>(impl){
}

android::status_t BpPingResponder::Ping(int32_t token, int32_t* _aidl_return) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInt32(token);
if (status != android::OK) { return status; }
status = remote()->transact(IPingResponder::PING, data, &reply);
if (status != android::OK) { return status; }
status = reply.readInt32(_aidl_return);
if (status != android::OK) { return status; }
return status;
}

}  // namespace generated

}  // namespace android
)";

const char kExpectedTrivialServerHeaderOutput[] =
R"(#ifndef AIDL_GENERATED__BN_PING_RESPONDER_H_
#define AIDL_GENERATED__BN_PING_RESPONDER_H_

#include <binder/IInterface.h>
#include <IPingResponder.h>

namespace android {

namespace generated {

class BnPingResponder : public android::BnInterface<IPingResponder> {
public:
android::status_t onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags = 0) override;
};  // class BnPingResponder

}  // namespace generated

}  // namespace android

#endif  // AIDL_GENERATED__BN_PING_RESPONDER_H_)";

const char kExpectedTrivialServerSourceOutput[] =
R"(#include <BnPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace generated {

android::status_t BnPingResponder::onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags) {
android::status_t status;
switch (code) {
case Call::PING:
{
int32_t in_token;
int32_t _aidl_return;
status = data.readInt32(&in_token);
if (status != android::OK) { break; }
status = Ping(in_token, &_aidl_return);
if (status != android::OK) { break; }
status = reply->writeInt32(_aidl_return);
if (status != android::OK) { break; }
}
break;
default:
{
status = android::BBinder::onTransact(code, data, reply, flags);
}
break;
}
return status;
}

}  // namespace generated

}  // namespace android
)";

const char kExpectedTrivialClientHeaderOutput[] =
R"(#ifndef AIDL_GENERATED__BP_PING_RESPONDER_H_
#define AIDL_GENERATED__BP_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <IPingResponder.h>

namespace android {

namespace generated {

class BpPingResponder : public android::BpInterface<IPingResponder> {
public:
explicit BpPingResponder(const android::sp<android::IBinder>& impl);
virtual ~BpPingResponder() = default;
android::status_t Ping(int32_t token, int32_t* _aidl_return) override;
};  // class BpPingResponder

}  // namespace generated

}  // namespace android

#endif  // AIDL_GENERATED__BP_PING_RESPONDER_H_)";

const char kExpectedTrivialInterfaceHeaderOutput[] =
R"(#ifndef AIDL_GENERATED__I_PING_RESPONDER_H_
#define AIDL_GENERATED__I_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>

namespace android {

namespace generated {

class IPingResponder : public android::IInterface {
public:
DECLARE_META_INTERFACE(PingResponder);
virtual android::status_t Ping(int32_t token, int32_t* _aidl_return) = 0;
enum Call {
  PING = android::IBinder::FIRST_CALL_TRANSACTION + 0,
};
};  // class IPingResponder

}  // namespace generated

}  // namespace android

#endif  // AIDL_GENERATED__I_PING_RESPONDER_H_)";

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
  AidlInterface* Parse() {

  FakeIoDelegate io_delegate;
  io_delegate.SetFileContents("IPingResponder.aidl", kTrivialInterfaceAIDL);

  cpp::TypeNamespace types;
  AidlInterface* ret = nullptr;
  std::vector<std::unique_ptr<AidlImport>> imports;
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
  AidlInterface* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientHeader(types, *interface);
  Compare(doc.get(), kExpectedTrivialClientHeaderOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesClientSource) {
  AidlInterface* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientSource(types, *interface);
  Compare(doc.get(), kExpectedTrivialClientSourceOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesServerHeader) {
  AidlInterface* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildServerHeader(types, *interface);
  Compare(doc.get(), kExpectedTrivialServerHeaderOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesServerSource) {
  AidlInterface* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildServerSource(types, *interface);
  Compare(doc.get(), kExpectedTrivialServerSourceOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesInterfaceHeader) {
  AidlInterface* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types, *interface);
  Compare(doc.get(), kExpectedTrivialInterfaceHeaderOutput);
}

TEST_F(TrivialInterfaceASTTest, GeneratesInterfaceSource) {
  AidlInterface* interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types, *interface);
  Compare(doc.get(), kExpectedTrivialInterfaceSourceOutput);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
