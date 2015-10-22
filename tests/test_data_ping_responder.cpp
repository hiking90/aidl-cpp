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

#include "tests/test_data.h"

namespace android {
namespace aidl {
namespace test_data {
namespace ping_responder {

const char kCanonicalName[] = "android.os.IPingResponder";
const char kInterfaceDefinition[] = R"(
package android.os;

interface IPingResponder {
  int Ping(int token);
}
)";

const char kGenHeaderDir[] = "some/path";
const char kGenInterfaceHeaderPath[] = "some/path/android/os/IPingResponder.h";
const char kGenClientHeaderPath[] = "some/path/android/os/BpPingResponder.h";
const char kGenServerHeaderPath[] = "some/path/android/os/BnPingResponder.h";

const char kExpectedCppOutput[] =
R"(#include <android/os/IPingResponder.h>
#include <android/os/BpPingResponder.h>

namespace android {

namespace os {

IMPLEMENT_META_INTERFACE(PingResponder, "android.os.IPingResponder");

}  // namespace os

}  // namespace android
#include <android/os/BpPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

BpPingResponder::BpPingResponder(const android::sp<android::IBinder>& impl)
    : BpInterface<IPingResponder>(impl){
}

android::status_t BpPingResponder::Ping(int32_t token, int32_t* _aidl_return) {
android::Parcel data;
android::Parcel reply;
android::status_t status;
status = data.writeInterfaceToken(getInterfaceDescriptor());
if (((status) != (android::OK))) {
return status;
}
status = data.writeInt32(token);
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
#include <android/os/BnPingResponder.h>
#include <binder/Parcel.h>

namespace android {

namespace os {

android::status_t BnPingResponder::onTransact(uint32_t code, const android::Parcel& data, android::Parcel* reply, uint32_t flags) {
android::status_t status;
switch (code) {
case Call::PING:
{
int32_t in_token;
int32_t _aidl_return;
if ((!data.checkInterface(this))) {
status = android::BAD_TYPE;
break;
}
status = data.readInt32(&in_token);
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

const char kExpectedIHeaderOutput[] =
R"(#ifndef AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_
#define AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <cstdint>
#include <utils/StrongPointer.h>

namespace android {

namespace os {

class IPingResponder : public android::IInterface {
public:
DECLARE_META_INTERFACE(PingResponder);
virtual android::status_t Ping(int32_t token, int32_t* _aidl_return) = 0;
enum Call {
  PING = android::IBinder::FIRST_CALL_TRANSACTION + 0,
};
};  // class IPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_I_PING_RESPONDER_H_)";

const char kExpectedBpHeaderOutput[] =
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
android::status_t Ping(int32_t token, int32_t* _aidl_return) override;
};  // class BpPingResponder

}  // namespace os

}  // namespace android

#endif  // AIDL_GENERATED_ANDROID_OS_BP_PING_RESPONDER_H_)";

const char kExpectedBnHeaderOutput[] =
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

}  // namespace ping_responder
}  // namespace test_data
}  // namespace aidl
}  // namespace android
