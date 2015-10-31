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
#include <utils/StrongPointer.h>

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
import foo.IFooType;
interface IComplexTypeInterface {
  int[] Send(in int[] goes_in, inout double[] goes_in_and_out, out boolean[] goes_out);
  oneway void Piff(int times);
  IFooType TakesABinder(IFooType f);
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
android::status_t Send(const std::vector<int32_t>& goes_in, std::vector<double>* goes_in_and_out, std::vector<bool>* goes_out, std::vector<int32_t>* _aidl_return) override;
android::status_t Piff(int32_t times) override;
android::status_t TakesABinder(const android::sp<::foo::IFooType>& f, android::sp<::foo::IFooType>* _aidl_return) override;
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

android::status_t BpComplexTypeInterface::Send(const std::vector<int32_t>& goes_in, std::vector<double>* goes_in_and_out, std::vector<bool>* goes_out, std::vector<int32_t>* _aidl_return) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInterfaceToken(getInterfaceDescriptor());
if (((status) != (android::OK))) {
return status;
}
status = data.writeInt32Vector(goes_in);
if (((status) != (android::OK))) {
return status;
}
status = data.writeDoubleVector(*goes_in_and_out);
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
status = reply.readInt32Vector(_aidl_return);
if (((status) != (android::OK))) {
return status;
}
status = reply.readDoubleVector(goes_in_and_out);
if (((status) != (android::OK))) {
return status;
}
status = reply.readBoolVector(goes_out);
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

android::status_t BpComplexTypeInterface::TakesABinder(const android::sp<::foo::IFooType>& f, android::sp<::foo::IFooType>* _aidl_return) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInterfaceToken(getInterfaceDescriptor());
if (((status) != (android::OK))) {
return status;
}
status = data.writeStrongBinder(IFooType::asBinder(f));
if (((status) != (android::OK))) {
return status;
}
status = remote()->transact(IComplexTypeInterface::TAKESABINDER, data, &reply);
if (((status) != (android::OK))) {
return status;
}
if (reply.readExceptionCode()) {
status = android::FAILED_TRANSACTION;
return status;
}
status = reply.readStrongBinder(_aidl_return);
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
std::vector<int32_t> in_goes_in;
std::vector<double> in_goes_in_and_out;
std::vector<bool> out_goes_out;
std::vector<int32_t> _aidl_return;
if ((!data.checkInterface(this))) {
status = android::BAD_TYPE;
break;
}
status = data.readInt32Vector(&in_goes_in);
if (((status) != (android::OK))) {
break;
}
status = data.readDoubleVector(&in_goes_in_and_out);
if (((status) != (android::OK))) {
break;
}
status = Send(in_goes_in, &in_goes_in_and_out, &out_goes_out, &_aidl_return);
if (((status) != (android::OK))) {
break;
}
status = reply->writeNoException();
if (((status) != (android::OK))) {
break;
}
status = reply->writeInt32Vector(_aidl_return);
if (((status) != (android::OK))) {
break;
}
status = reply->writeDoubleVector(in_goes_in_and_out);
if (((status) != (android::OK))) {
break;
}
status = reply->writeBoolVector(out_goes_out);
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
case Call::TAKESABINDER:
{
android::sp<::foo::IFooType> in_f;
android::sp<::foo::IFooType> _aidl_return;
if ((!data.checkInterface(this))) {
status = android::BAD_TYPE;
break;
}
status = data.readStrongBinder(&in_f);
if (((status) != (android::OK))) {
break;
}
status = TakesABinder(in_f, &_aidl_return);
if (((status) != (android::OK))) {
break;
}
status = reply->writeNoException();
if (((status) != (android::OK))) {
break;
}
status = reply->writeStrongBinder(IFooType::asBinder(_aidl_return));
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
#include <foo/IFooType.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

namespace os {

class IComplexTypeInterface : public android::IInterface {
public:
DECLARE_META_INTERFACE(ComplexTypeInterface);
virtual android::status_t Send(const std::vector<int32_t>& goes_in, std::vector<double>* goes_in_and_out, std::vector<bool>* goes_out, std::vector<int32_t>* _aidl_return) = 0;
virtual android::status_t Piff(int32_t times) = 0;
virtual android::status_t TakesABinder(const android::sp<::foo::IFooType>& f, android::sp<::foo::IFooType>* _aidl_return) = 0;
enum Call {
  SEND = android::IBinder::FIRST_CALL_TRANSACTION + 0,
  PIFF = android::IBinder::FIRST_CALL_TRANSACTION + 1,
  TAKESABINDER = android::IBinder::FIRST_CALL_TRANSACTION + 2,
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
  ASTTest(string file_path, string file_contents)
      : file_path_(file_path),
        file_contents_(file_contents) {}

  unique_ptr<AidlInterface> Parse() {
    io_delegate_.SetFileContents(file_path_, file_contents_);

    unique_ptr<AidlInterface> ret;
    std::vector<std::unique_ptr<AidlImport>> imports;
    int err = ::android::aidl::internals::load_and_validate_aidl(
        {},  // no preprocessed files
        {"."},
        file_path_,
        io_delegate_,
        &types_,
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

  const string file_path_;
  const string file_contents_;
  FakeIoDelegate io_delegate_;
  TypeNamespace types_;
};

class PrimitiveInterfaceASTTest : public ASTTest {
 public:
  PrimitiveInterfaceASTTest()
      : ASTTest("android/os/IPingResponder.aidl",
                kPrimitiveInterfaceAIDL) {}
};

TEST_F(PrimitiveInterfaceASTTest, GeneratesClientHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildClientHeader(types_, *interface);
  Compare(doc.get(), kExpectedPrimitiveClientHeaderOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesClientSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildClientSource(types_, *interface);
  Compare(doc.get(), kExpectedPrimitiveClientSourceOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesServerHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildServerHeader(types_, *interface);
  Compare(doc.get(), kExpectedPrimitiveServerHeaderOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesServerSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildServerSource(types_, *interface);
  Compare(doc.get(), kExpectedPrimitiveServerSourceOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesInterfaceHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types_, *interface);
  Compare(doc.get(), kExpectedPrimitiveInterfaceHeaderOutput);
}

TEST_F(PrimitiveInterfaceASTTest, GeneratesInterfaceSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types_, *interface);
  Compare(doc.get(), kExpectedPrimitiveInterfaceSourceOutput);
}

class ComplexTypeInterfaceASTTest : public ASTTest {
 public:
  ComplexTypeInterfaceASTTest()
      : ASTTest("android/os/IComplexTypeInterface.aidl",
                kComplexTypeInterfaceAIDL) {
    io_delegate_.SetFileContents("foo/IFooType.aidl",
                                 "package foo; interface IFooType {}");
  }
};

TEST_F(ComplexTypeInterfaceASTTest, GeneratesClientHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildClientHeader(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeClientHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesClientSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildClientSource(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeClientSourceOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesServerHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildServerHeader(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeServerHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesServerSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildServerSource(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeServerSourceOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesInterfaceHeader) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildInterfaceHeader(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeInterfaceHeaderOutput);
}

TEST_F(ComplexTypeInterfaceASTTest, GeneratesInterfaceSource) {
  unique_ptr<AidlInterface> interface = Parse();
  ASSERT_NE(interface, nullptr);
  unique_ptr<Document> doc = internals::BuildInterfaceSource(types_, *interface);
  Compare(doc.get(), kExpectedComplexTypeInterfaceSourceOutput);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android
