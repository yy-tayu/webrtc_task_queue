/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "task_queue_base.h"

#include "absl/base/attributes.h"
#include "absl/base/config.h"
#include "absl/functional/any_invocable.h"

namespace webrtc {
namespace {
ABSL_CONST_INIT thread_local TaskQueueBase* current = nullptr;
}  // namespace

TaskQueueBase* TaskQueueBase::Current() {
  return current;
}

TaskQueueBase::CurrentTaskQueueSetter::CurrentTaskQueueSetter(TaskQueueBase* task_queue)
  : previous_(current) {
  current = task_queue;
}

TaskQueueBase::CurrentTaskQueueSetter::~CurrentTaskQueueSetter() {
  current = previous_;
}
}  // namespace webrtc

namespace webrtc {
void TaskQueueBase::PostTask(absl::AnyInvocable<void() &&> task) {
}
void TaskQueueBase::PostDelayedTask(absl::AnyInvocable<void() &&> task, int ms) {
}
void TaskQueueBase::PostDelayedHighPrecisionTask(absl::AnyInvocable<void() &&> task, int ms) {
}
}  // namespace webrtc
