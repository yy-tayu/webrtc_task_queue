/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef API_TASK_QUEUE_QUEUED_TASK_H_
#define API_TASK_QUEUE_QUEUED_TASK_H_

namespace webrtc {
class QueuedTask {
 public:
  virtual ~QueuedTask() = default;
  virtual bool Run() = 0;
};
}  // namespace webrtc
#endif  // API_TASK_QUEUE_QUEUED_TASK_H_
