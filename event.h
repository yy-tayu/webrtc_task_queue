/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_EVENT_H_
#define RTC_BASE_EVENT_H_

#include <windows.h>

namespace rtc {

class Event {
 public:
  static const int kForever = -1;
  Event();
  Event(bool manual_reset, bool initially_signaled);
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;
  ~Event();

  void Set();
  void Reset();
  bool Wait(int give_up_after_ms, int warn_after_ms);
  bool Wait(int give_up_after_ms) {
    return Wait(give_up_after_ms,
                give_up_after_ms == kForever ? 3000 : kForever);
  }
 private:
  HANDLE event_handle_;
};

class ScopedAllowBaseSyncPrimitives {
 public:
  ScopedAllowBaseSyncPrimitives() {}
  ~ScopedAllowBaseSyncPrimitives() {}
};

class ScopedAllowBaseSyncPrimitivesForTesting {
 public:
  ScopedAllowBaseSyncPrimitivesForTesting() {}
  ~ScopedAllowBaseSyncPrimitivesForTesting() {}
};
}  // namespace rtc

#endif  // RTC_BASE_EVENT_H_
