/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef API_TASK_QUEUE_TASK_QUEUE_BASE_H_
#define API_TASK_QUEUE_TASK_QUEUE_BASE_H_

#include <memory>
#include <utility>

#include "absl/functional/any_invocable.h"
#include "queued_task.h"

namespace webrtc {
class TaskQueueBase {
 public:
  enum class DelayPrecision {
    kLow,
    kHigh,
  };

  virtual void Delete() = 0;
  virtual void PostTask(absl::AnyInvocable<void() &&> task); // override
  virtual void PostDelayedTask(absl::AnyInvocable<void() &&> task, int ms); // override
  virtual void PostDelayedHighPrecisionTask(absl::AnyInvocable<void() &&> task, int ms); // override
  void PostDelayedTaskWithPrecision(DelayPrecision precision, absl::AnyInvocable<void() &&> task, int ms) {
    switch (precision) {
      case DelayPrecision::kLow:
        PostDelayedTask(std::move(task), ms);
        break;
      case DelayPrecision::kHigh:
        PostDelayedHighPrecisionTask(std::move(task), ms);
        break;
    }
  }
  static TaskQueueBase* Current();
  bool IsCurrent() const { return Current() == this; }
 protected:
  class CurrentTaskQueueSetter {
   public:
    explicit CurrentTaskQueueSetter(TaskQueueBase* task_queue);
    CurrentTaskQueueSetter(const CurrentTaskQueueSetter&) = delete;
    CurrentTaskQueueSetter& operator=(const CurrentTaskQueueSetter&) = delete;
    ~CurrentTaskQueueSetter();
   private:
    TaskQueueBase* const previous_;
  };
  virtual ~TaskQueueBase() = default;
};

struct TaskQueueDeleter {
  void operator()(TaskQueueBase* task_queue) const { task_queue->Delete(); }
};
}  // namespace webrtc
#endif  // API_TASK_QUEUE_TASK_QUEUE_BASE_H_