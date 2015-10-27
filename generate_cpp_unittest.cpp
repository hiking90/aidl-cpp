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
#include "tests/test_util.h"
#include "type_cpp.h"

using android::aidl::test::FakeIoDelegate;
using std::string;
using std::unique_ptr;

namespace android {
namespace aidl {
namespace cpp {
namespace {

const string kPrimitiveInterfaceAIDL =
R"(
package android.os;

interface IPingResponder {
  int Ping(String token);
})";

const char kExpectedPrimitiveClientSourceOutput[] =
R"(#include <android/os/BpPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

BpPingResponder::BpPingResponder(const android::sp<android::IBinder>& impl)
    : BpInterface<IPingResponder>(impl){
}

android::status_t BpPingResponder::Ping(const android::String16& token, int32_t* _aidl_return) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInterfaceToken(getInterfaceDescriptor());
if (((status) != (android::OK))) {
return status;
}
status = data.writeString16(token);
if (((status) != (android::OK))) {
return status;
}
status = remote()->transact(IPingResponder::PING, data, &reply);
if (((status) != (android::OK))) {
return status;
}
if (reply.readExceptionCode()) {
status = android::FAILED_TRANSACTION;
return status;
}
status = reply.readInt32(_aidl_return);
if (((status) != (android::OK))) {
return status;
}
return status;
}

}  // namespace os

}  // namespace android
)";

const char kExpectedPrimitiveServerHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BN_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_BN_PING_RESPONDER_H_

#include <binder/IInterface.h>
#include <android/os/IPingResponder.h>

namespace android {

namespace os {

class BnPingResponder : public android::BnInterface<IPingResponder> {
public:
android::status_t onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags = 0) override;
};  // class BnPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BN_PING_RESPONDER_H_)";

const char kExpectedPrimitiveServerSourceOutput[] =
R"(#include <android/os/BnPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

android::status_t BnPingResponder::onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags) {
android::status_t status;
switch (code) {
case Call::PING:
{
android::String16 in_token;
int32_t _aidl_return;
if ((!data.checkInterface(this))) {
status = android::BAD_TYPE;
break;
}
status = data.readString16(&in_token);
if (((status) != (android::OK))) {
break;
}
status = Ping(in_token, &_aidl_return);
if (((status) != (android::OK))) {
break;
}
status = reply->writeNoException();
if (((status) != (android::OK))) {
break;
}
status = reply->writeInt32(_aidl_return);
if (((status) != (android::OK))) {
break;
}
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

}  // namespace os

}  // namespace android
)";

const char kExpectedPrimitiveClientHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/os/IPingResponder.h>

namespace android {

namespace os {

class BpPingResponder : public android::BpInterface<IPingResponder> {
public:
explicit BpPingResponder(const android::sp<android::IBinder>& impl);
virtual ~BpPingResponder() = default;
android::status_t Ping(const android::String16& token, int32_t* _aidl_return) override;
};  // class BpPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_)";

const char kExpectedPrimitiveInterfaceHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <cstdint>
#include <utils/String16.h>

namespace android {

namespace os {

class IPingResponder : public android::IInterface {
public:
DECLARE_META_INTERFACE(PingResponder);
virtual android::status_t Ping(const android::String16& token, int32_t* _aidl_return) = 0;
enum Call {
  PING = android::IBinder::FIRST_CALL_TRANSACTION + 0,
};
};  // class IPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_)";

const char kExpectedPrimitiveInterfaceSourceOutput[] =
R"(#include <android/os/IPingResponder.h>
#include <android/os/BpPingResponder.h>

namespace android {

namespace os {

IMPLEMENT_META_INTERFACE(PingResponder, "android.os.IPingResponder");

}  // namespace os

}  // namespace android
)";

const string kComplexTypeInterfaceAIDL =
R"(package android.os;
interface IComplexTypeInterface {
  int Send(in int[] token, out boolean[] item);
  oneway void Piff(int times);
})";

const char kExpectedComplexTypeClientHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BP_COMPLEX_TYPE_INTERFACE_H_
#define AIDL_GENERATED_ANDROID_OS_BP_COMPLEX_TYPE_INTERFACE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/Errors.h>
#include <android/os/IComplexTypeInterface.h>

namespace android {

namespace os {

class BpComplexTypeInterface : public android::BpInterface<IComplexTypeInterface> {
public:
explicit BpComplexTypeInterface(const android::sp<android::IBinder>& impl);
virtual ~BpComplexTypeInterface() = default;
android::status_t Send(const std::vector<int32_t>& token, std::vector<bool>* item, int32_t* _aidl_return) override;
android::status_t Piff(int32_t times) override;
};  // class BpComplexTypeInterface

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BP_COMPLEX_TYPE_INTERFACE_H_)";

const char kExpectedComplexTypeClientSourceOutput[] =
R"(#include <android/os/BpComplexTypeInterface.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

BpComplexTypeInterface::BpComplexTypeInterface(const android::sp<android::IBinder>& impl)
    : BpInterface<IComplexTypeInterface>(impl){
}

android::status_t BpComplexTypeInterface::Send(const std::vector<int32_t>& token, std::vector<bool>* item, int32_t* _aidl_return) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInterfaceToken(getInterfaceDescriptor());
if (((status) != (android::OK))) {
return status;
}
status = data.writeInt32Vector(token);
if (((status) != (android::OK))) {
return status;
}
status = remote()->transact(IComplexTypeInterface::SEND, data, &reply);
if (((status) != (android::OK))) {
return status;
}
if (reply.readExceptionCode()) {
status = android::FAILED_TRANSACTION;
return status;
}
status = reply.readInt32(_aidl_return);
if (((status) != (android::OK))) {
return status;
}
status = reply.readBoolVector(item);
if (((status) != (android::OK))) {
return status;
}
return status;
}

android::status_t BpComplexTypeInterface::Piff(int32_t times) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInterfaceToken(getInterfaceDescriptor());
if (((status) != (android::OK))) {
return status;
}
status = data.writeInt32(times);
if (((status) != (android::OK))) {
return status;
}
status = remote()->transact(IComplexTypeInterface::PIFF, data, &reply, android::IBinder::FLAG_ONEWAY);
if (((status) != (android::OK))) {
return status;
}
return status;
}

}  // namespace os

}  // namespace android
)";

const char kExpectedComplexTypeServerHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_BN_COMPLEX_TYPE_INTERFACE_H_
#define AIDL_GENERATED_ANDROID_OS_BN_COMPLEX_TYPE_INTERFACE_H_

#include <binder/IInterface.h>
#include <android/os/IComplexTypeInterface.h>

namespace android {

namespace os {

class BnComplexTypeInterface : public android::BnInterface<IComplexTypeInterface> {
public:
android::status_t onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags = 0) override;
};  // class BnComplexTypeInterface

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BN_COMPLEX_TYPE_INTERFACE_H_)";

const char kExpectedComplexTypeServerSourceOutput[] =
R"(#include <android/os/BnComplexTypeInterface.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

android::status_t BnComplexTypeInterface::onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags) {
android::status_t status;
switch (code) {
case Call::SEND:
{
std::vector<int32_t> in_token;
std::vector<bool> out_item;
int32_t _aidl_return;
if ((!data.checkInterface(this))) {
status = android::BAD_TYPE;
break;
}
status = data.readInt32Vector(&in_token);
if (((status) != (android::OK))) {
break;
}
status = Send(in_token, &out_item, &_aidl_return);
if (((status) != (android::OK))) {
break;
}
status = reply->writeNoException();
if (((status) != (android::OK))) {
break;
}
status = reply->writeInt32(_aidl_return);
if (((status) != (android::OK))) {
break;
}
status = reply->writeBoolVector(out_item);
if (((status) != (android::OK))) {
break;
}
}
break;
case Call::PIFF:
{
int32_t in_times;
if ((!data.checkInterface(this))) {
status = android::BAD_TYPE;
break;
}
status = data.readInt32(&in_times);
if (((status) != (android::OK))) {
break;
}
status = Piff(in_times);
if (((status) != (android::OK))) {
break;
}
status = reply->writeNoException();
if (((status) != (android::OK))) {
break;
}
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

}  // namespace os

}  // namespace android
)";

const char kExpectedComplexTypeInterfaceHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_I_COMPLEX_TYPE_INTERFACE_H_
#define AIDL_GENERATED_ANDROID_OS_I_COMPLEX_TYPE_INTERFACE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <cstdint>
#include <vector>

namespace android {

namespace os {

class IComplexTypeInterface : public android::IInterface {
public:
DECLARE_META_INTERFACE(ComplexTypeInterface);
virtual android::status_t Send(const std::vector<int32_t>& token, std::vector<bool>* item, int32_t* _aidl_return) = 0;
virtual android::status_t Piff(int32_t times) = 0;
enum Call {
  SEND = android::IBinder::FIRST_CALL_TRANSACTION + 0,
  PIFF = android::IBinder::FIRST_CALL_TRANSACTION + 1,
};
};  // class IComplexTypeInterface

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_I_COMPLEX_TYPE_INTERFACE_H_)";

const char kExpectedComplexTypeInterfaceSourceOutput[] =
R"(#include <android/os/IComplexTypeInterface.h>
#include <android/os/BpComplexTypeInterface.h>

namespace android {

namespace os {

IMPLEMENT_META_INTERFACE(ComplexTypeInterface, "android.os.IComplexTypeInterface");

}  // namespace os

}  // namespace android
)";

}  // namespace

class ASTTest : public ::testing::Test {
 protected:
  virtual const string& FilePath() = 0;
  virtual const string& FileContents() = 0;

  unique_ptr<AidlInterface> Parse() {
    FakeIoDelegate io_delegate;
    io_delegate.SetFileContents(FilePath(), FileContents());

    cpp::TypeNamespace types;
    unique_ptr<AidlInterface> ret;
    std::vector<std::unique_ptr<AidlImport>> imports;
    int err = ::android::aidl::internals::load_and_validate_aidl(
        {},  // no preprocessed files
        {},  // no import paths
        FilePath(),
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

    if (expected == output) {
      return; // Success
    }

    test::PrintDiff(expected, output);
    FAIL() << "Document contents did not match expected contents";
  }
};

class PrimitiveInterfaceASTTest : public ASTTest {
 protected:
  const string fp_ = "android/os/IPingResponder.aidl";
  const string& FilePath() override { return fp_; }
  const string& FileContents() override { return kPrimitiveInterfaceAIDL; }
};

TEST_F(PrimitiveInterfaceASTTest, GeneratesClientHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientHeader(types, *interface);
  Compare(doc.get(), kExpectedPrimitiveClientHeaderOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesClientSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientSource(types, *interface);
  Compare(doc.get(), kExpectedPrimitiveClientSourceOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesServerHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildServerHeader(types, *interface);
  Compare(doc.get(), kExpectedPrimitiveServerHeaderOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesServerSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildServerSource(types, *interface);
  Compare(doc.get(), kExpectedPrimitiveServerSourceOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesInterfaceHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types, *interface);
  Compare(doc.get(), kExpectedPrimitiveInterfaceHeaderOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesInterfaceSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types, *interface);
  Compare(doc.get(), kExpectedPrimitiveInterfaceSourceOutput);
}

class ComplexTypeInterfaceASTTest : public ASTTest {
 protected:
  const string fp_ = "android/os/IComplexTypeInterface.aidl";
  const string& FilePath() override { return fp_; }
  const string& FileContents() override { return kComplexTypeInterfaceAIDL; }
};

TEST_F(ComplexTypeInterfaceASTTest, GeneratesClientHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientHeader(types, *interface);
  Compare(doc.get(), kExpectedComplexTypeClientHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesClientSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildClientSource(types, *interface);
  Compare(doc.get(), kExpectedComplexTypeClientSourceOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesServerHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildServerHeader(types, *interface);
  Compare(doc.get(), kExpectedComplexTypeServerHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesServerSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildServerSource(types, *interface);
  Compare(doc.get(), kExpectedComplexTypeServerSourceOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesInterfaceHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types, *interface);
  Compare(doc.get(), kExpectedComplexTypeInterfaceHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesInterfaceSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  TypeNamespace types;
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types, *interface);
  Compare(doc.get(), kExpectedComplexTypeInterfaceSourceOutput);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
