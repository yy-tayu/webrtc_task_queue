/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_PLATFORM_THREAD_H_
#define RTC_BASE_PLATFORM_THREAD_H_

#include <functional>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "platform_thread_types.h"

namespace rtc {

enum class ThreadPriority {
  kLow = 1,
  kNormal,
  kHigh,
  kRealtime,
};

struct ThreadAttributes {
  ThreadPriority priority = ThreadPriority::kNormal;
  ThreadAttributes& SetPriority(ThreadPriority priority_param) {
    priority = priority_param;
    return *this;
  }
};

// Represents a simple worker thread.
class PlatformThread final {
 public:
  using Handle = HANDLE;
  PlatformThread() = default;
  PlatformThread(PlatformThread&& rhs);
  PlatformThread(const PlatformThread&) = delete;
  PlatformThread& operator=(const PlatformThread&) = delete;
  PlatformThread& operator=(PlatformThread&& rhs);
  virtual ~PlatformThread();
  void Finalize();
  bool empty() const { return !handle_.has_value(); }
  static PlatformThread SpawnJoinable(std::function<void()> thread_function, absl::string_view name, ThreadAttributes attributes = ThreadAttributes());
  static PlatformThread SpawnDetached(std::function<void()> thread_function, absl::string_view name, ThreadAttributes attributes = ThreadAttributes());
  absl::optional<Handle> GetHandle() const;
  bool QueueAPC(PAPCFUNC apc_function, ULONG_PTR data);
 private:
  PlatformThread(Handle handle, bool joinable);
  static PlatformThread SpawnThread(std::function<void()> thread_function, absl::string_view name, ThreadAttributes attributes, bool joinable);
  absl::optional<Handle> handle_;
  bool joinable_ = false;
};

}  // namespace rtc

#endif  // RTC_BASE_PLATFORM_THREAD_H_
