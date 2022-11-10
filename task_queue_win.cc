/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "task_queue_win.h"
#include <winsock2.h>
#define NOMINMAX
#include <windows.h>
#include <sal.h>       // Must come after windows headers.
#include <mmsystem.h>  // Must come after windows headers.
#include <string.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <utility>
#include <chrono>

#include "absl/functional/any_invocable.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "task_queue_base.h"
#include "arraysize.h"
#include "event.h"
#include "platform_thread.h"


namespace webrtc {
namespace {
#define WM_QUEUE_DELAYED_TASK WM_USER + 2

void CALLBACK InitializeQueueThread(ULONG_PTR param) {
  MSG msg;
  ::PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
  rtc::Event* data = reinterpret_cast<rtc::Event*>(param);
  data->Set();
}

rtc::ThreadPriority TaskQueuePriorityToThreadPriority(
  TaskQueueFactory::Priority priority) {
  switch (priority) {
  case TaskQueueFactory::Priority::HIGH:
    return rtc::ThreadPriority::kRealtime;
  case TaskQueueFactory::Priority::LOW:
    return rtc::ThreadPriority::kLow;
  case TaskQueueFactory::Priority::NORMAL:
    return rtc::ThreadPriority::kNormal;
  }
}

int64_t CurrentTime() {
  auto duration_now = std::chrono::system_clock::now().time_since_epoch();
  int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(duration_now).count();
  return time;
}

class DelayedTaskInfo {
 public:
  DelayedTaskInfo() {}
  DelayedTaskInfo(std::chrono::milliseconds delay, absl::AnyInvocable<void()&&> task)
    :task_(std::move(task))
  {
  }
  DelayedTaskInfo(int delay, absl::AnyInvocable<void() &&> task)
    : delay_time_(delay), task_(std::move(task)), create_time_(std::chrono::system_clock::now()){}
  DelayedTaskInfo(DelayedTaskInfo&&) = default;
  bool operator>(const DelayedTaskInfo& other) const {
    return due_time() > other.due_time();
  }
  DelayedTaskInfo& operator=(DelayedTaskInfo&& other) = default;
  void Run() const {
    std::move(task_)();
  }
  int64_t due_time() const
  {
    auto duration = create_time_.time_since_epoch();
    int64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis + delay_time_.count();
  }

 private:
  std::chrono::milliseconds delay_time_;
  std::chrono::time_point<std::chrono::system_clock> create_time_ = std::chrono::system_clock::now();
  mutable absl::AnyInvocable<void() &&> task_;
};

class MultimediaTimer {
 public:
  MultimediaTimer() : event_(::CreateEvent(nullptr, true, false, nullptr)) {}
  ~MultimediaTimer() {
    Cancel();
    ::CloseHandle(event_);
  }

  MultimediaTimer(const MultimediaTimer&) = delete;
  MultimediaTimer& operator=(const MultimediaTimer&) = delete;
  bool StartOneShotTimer(UINT delay_ms) {
    timer_id_ = ::timeSetEvent(delay_ms, 0, reinterpret_cast<LPTIMECALLBACK>(event_), 0,
                       TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
    return timer_id_ != 0;
  }

  void Cancel() {
    if (timer_id_) {
      ::timeKillEvent(timer_id_);
      timer_id_ = 0;
    }
    ::ResetEvent(event_);
  }

  HANDLE* event_for_wait() { return &event_; }
 private:
  HANDLE event_ = nullptr;
  MMRESULT timer_id_ = 0;
};

class TaskQueueWin : public TaskQueueBase {
 public:
  TaskQueueWin(absl::string_view queue_name, rtc::ThreadPriority priority);
  ~TaskQueueWin() override = default;

  virtual void Delete() override;
  virtual void PostTask(absl::AnyInvocable<void() &&> task) override;
  virtual void PostDelayedTask(absl::AnyInvocable<void() &&> task, int delay) override;
  virtual void PostDelayedHighPrecisionTask(absl::AnyInvocable<void() &&> task, int delay) override;
  void RunPendingTasks();
 private:
  void RunThreadMain();
  bool ProcessQueuedMessages();
  void RunDueTasks();
  void ScheduleNextTimer();
  void CancelTimers();

  MultimediaTimer timer_;
  std::priority_queue<DelayedTaskInfo, std::vector<DelayedTaskInfo>, std::greater<DelayedTaskInfo>> timer_tasks_;
  UINT_PTR timer_id_ = 0;
  rtc::PlatformThread thread_;
  std::queue<absl::AnyInvocable<void() &&>> pending_;
  HANDLE in_queue_;
};

TaskQueueWin::TaskQueueWin(absl::string_view queue_name, rtc::ThreadPriority priority)
  : in_queue_(::CreateEvent(nullptr, true, false, nullptr)) {
  thread_ = rtc::PlatformThread::SpawnJoinable([this] { RunThreadMain(); }, queue_name, rtc::ThreadAttributes().SetPriority(priority));
  rtc::Event event(false, false);
  thread_.QueueAPC(&InitializeQueueThread, reinterpret_cast<ULONG_PTR>(&event));
  event.Wait(rtc::Event::kForever);
}

void TaskQueueWin::Delete() {
  while (!::PostThreadMessage(GetThreadId(*thread_.GetHandle()), WM_QUIT, 0, 0)) {
    Sleep(1);
  }
  thread_.Finalize();
  ::CloseHandle(in_queue_);
  delete this;
}

void TaskQueueWin::PostTask(absl::AnyInvocable<void() &&> task) {
  pending_.push(std::move(task));
  ::SetEvent(in_queue_);
}

void TaskQueueWin::PostDelayedTask(absl::AnyInvocable<void() &&> task, int delay) {
  if (delay <= 0) {
    PostTask(std::move(task));
    return;
  }
  auto* task_info = new DelayedTaskInfo(delay, std::move(task));
  if (!::PostThreadMessage(GetThreadId(*thread_.GetHandle()), WM_QUEUE_DELAYED_TASK, 0, reinterpret_cast<LPARAM>(task_info))) {
    delete task_info;
  }
}

void TaskQueueWin::PostDelayedHighPrecisionTask(absl::AnyInvocable<void() &&> task, int delay) {
  PostDelayedTask(std::move(task), delay);
}

void TaskQueueWin::RunPendingTasks() {
  while (true) {
    absl::AnyInvocable<void() &&> task;
    {
      if (pending_.empty())
        break;
      task = std::move(pending_.front());
      pending_.pop();
    }

    std::move(task)();
  }
}

void TaskQueueWin::RunThreadMain() {
  CurrentTaskQueueSetter set_current(this);
  HANDLE handles[2] = {*timer_.event_for_wait(), in_queue_};
  while (true) {
    DWORD result = ::MsgWaitForMultipleObjectsEx(2, handles, INFINITE, QS_ALLEVENTS, MWMO_ALERTABLE);
    if (result == (WAIT_OBJECT_0 + 2)) {
      if (!ProcessQueuedMessages())
        break;
    }

    if (result == WAIT_OBJECT_0 || (!timer_tasks_.empty() && ::WaitForSingleObject(*timer_.event_for_wait(), 0) == WAIT_OBJECT_0)) {
      timer_.Cancel();
      RunDueTasks();
      ScheduleNextTimer();
    }

    if (result == (WAIT_OBJECT_0 + 1)) {
      ::ResetEvent(in_queue_);
      RunPendingTasks();
    }
  }
}

bool TaskQueueWin::ProcessQueuedMessages() {
  MSG msg = {};
  static constexpr std::chrono::milliseconds kMaxTaskProcessingTime(500);
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  std::chrono::milliseconds start = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
  while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) && msg.message != WM_QUIT) {
    if (!msg.hwnd) {
      switch (msg.message) {
        case WM_QUEUE_DELAYED_TASK: {
          std::unique_ptr<DelayedTaskInfo> info(
              reinterpret_cast<DelayedTaskInfo*>(msg.lParam));
          bool need_to_schedule_timers =
              timer_tasks_.empty() ||
              timer_tasks_.top().due_time() > info->due_time();
          timer_tasks_.push(std::move(*info));
          if (need_to_schedule_timers) {
            CancelTimers();
            ScheduleNextTimer();
          }
          break;
        }
        case WM_TIMER: {
          ::KillTimer(nullptr, msg.wParam);
          timer_id_ = 0;
          RunDueTasks();
          ScheduleNextTimer();
          break;
        }
        default:
          break;
      }
    } else {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
    auto duration_now = std::chrono::system_clock::now().time_since_epoch();
    std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    if (now > start + kMaxTaskProcessingTime)
      break;
  }
  return msg.message != WM_QUIT;
}

void TaskQueueWin::RunDueTasks() {
  auto now = CurrentTime();
  do {
    const auto& top = timer_tasks_.top();
    if (top.due_time() > now)
      break;
    top.Run();
    timer_tasks_.pop();
  } while (!timer_tasks_.empty());
}

void TaskQueueWin::ScheduleNextTimer() {
  if (timer_tasks_.empty())
    return;

  const auto& next_task = timer_tasks_.top();
  int64_t delay = next_task.due_time() - CurrentTime();
  delay = 0 > delay ? 0 : delay;
  uint32_t milliseconds = 5000;
  if (!timer_.StartOneShotTimer(delay))
    timer_id_ = ::SetTimer(nullptr, 0, delay, nullptr);
}

void TaskQueueWin::CancelTimers() {
  timer_.Cancel();
  if (timer_id_) {
    ::KillTimer(nullptr, timer_id_);
    timer_id_ = 0;
  }
}

class TaskQueueWinFactory : public TaskQueueFactory {
 public:
  std::unique_ptr<TaskQueueBase, TaskQueueDeleter> CreateTaskQueue(absl::string_view name,
    Priority priority) const override {
    return std::unique_ptr<TaskQueueBase, TaskQueueDeleter>(
        new TaskQueueWin(name, TaskQueuePriorityToThreadPriority(priority)));
  }
};
}  // namespace

std::unique_ptr<TaskQueueFactory> CreateTaskQueueWinFactory() {
  return std::make_unique<TaskQueueWinFactory>();
}
}  // namespace webrtc