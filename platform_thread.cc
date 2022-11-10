/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "platform_thread.h"

#include <algorithm>
#include <memory>

namespace rtc {
namespace {

int Win32PriorityFromThreadPriority(ThreadPriority priority) {
  switch (priority) {
    case ThreadPriority::kLow:
      return THREAD_PRIORITY_BELOW_NORMAL;
    case ThreadPriority::kNormal:
      return THREAD_PRIORITY_NORMAL;
    case ThreadPriority::kHigh:
      return THREAD_PRIORITY_ABOVE_NORMAL;
    case ThreadPriority::kRealtime:
      return THREAD_PRIORITY_TIME_CRITICAL;
  }
  return THREAD_PRIORITY_BELOW_NORMAL;
}

bool SetPriority(ThreadPriority priority) {
  return SetThreadPriority(GetCurrentThread(), Win32PriorityFromThreadPriority(priority)) != FALSE;
}

DWORD WINAPI RunPlatformThread(void* param) {
  ::SetLastError(ERROR_SUCCESS);
  auto function = static_cast<std::function<void()>*>(param);
  (*function)();
  delete function;
  return 0;
}
}  // namespace

PlatformThread::PlatformThread(Handle handle, bool joinable)
  : handle_(handle), joinable_(joinable) {}

PlatformThread::PlatformThread(PlatformThread&& rhs)
  : handle_(rhs.handle_), joinable_(rhs.joinable_) {
  rhs.handle_ = absl::nullopt;
}

PlatformThread& PlatformThread::operator=(PlatformThread&& rhs) {
  Finalize();
  handle_ = rhs.handle_;
  joinable_ = rhs.joinable_;
  rhs.handle_ = absl::nullopt;
  return *this;
}

PlatformThread::~PlatformThread() {
  Finalize();
}

PlatformThread PlatformThread::SpawnJoinable( std::function<void()> thread_function, absl::string_view name, ThreadAttributes attributes) {
  return SpawnThread(std::move(thread_function), name, attributes,/*joinable=*/true);
}

PlatformThread PlatformThread::SpawnDetached(std::function<void()> thread_function, absl::string_view name, ThreadAttributes attributes) {
  return SpawnThread(std::move(thread_function), name, attributes, /*joinable=*/false);
}

absl::optional<PlatformThread::Handle> PlatformThread::GetHandle() const {
  return handle_;
}

bool PlatformThread::QueueAPC(PAPCFUNC function, ULONG_PTR data) {
  return handle_.has_value() ? QueueUserAPC(function, *handle_, data) != FALSE : false;
}

void PlatformThread::Finalize() {
  if (!handle_.has_value())
    return;
  if (joinable_)
    WaitForSingleObject(*handle_, INFINITE);
  CloseHandle(*handle_);
  handle_ = absl::nullopt;
}

PlatformThread PlatformThread::SpawnThread( std::function<void()> thread_function, absl::string_view name, ThreadAttributes attributes, bool joinable) {
  auto start_thread_function_ptr = new std::function<void()>(
    [thread_function = std::move(thread_function), name = std::string(name), attributes] {
        rtc::SetCurrentThreadName(name.c_str());
        SetPriority(attributes.priority);
        thread_function();
    });
  DWORD thread_id = 0;
  PlatformThread::Handle handle = ::CreateThread(nullptr, 1024 * 1024, &RunPlatformThread, start_thread_function_ptr, STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id);
  return PlatformThread(handle, joinable);
}
}  // namespace rtc
