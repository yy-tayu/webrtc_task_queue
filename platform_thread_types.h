/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_PLATFORM_THREAD_TYPES_H_
#define RTC_BASE_PLATFORM_THREAD_TYPES_H_

#include <winsock2.h>
#include <windows.h>

namespace rtc {

typedef DWORD PlatformThreadId;
typedef DWORD PlatformThreadRef;

PlatformThreadId CurrentThreadId();

PlatformThreadRef CurrentThreadRef();

bool IsThreadRefEqual(const PlatformThreadRef& a, const PlatformThreadRef& b);

void SetCurrentThreadName(const char* name);

}  // namespace rtc
#endif  // RTC_BASE_PLATFORM_THREAD_TYPES_H_
