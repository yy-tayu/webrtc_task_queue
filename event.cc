#include "event.h"
#include <windows.h>
#include "absl/types/optional.h"

namespace rtc {

Event::Event() : Event(false, false) {}

Event::Event(bool manual_reset, bool initially_signaled) {
  event_handle_ = ::CreateEvent(nullptr,  // Security attributes.
                                manual_reset, initially_signaled,
                                nullptr);  // Name.
}

Event::~Event() {
  CloseHandle(event_handle_);
}

void Event::Set() {
  SetEvent(event_handle_);
}

void Event::Reset() {
  ResetEvent(event_handle_);
}

bool Event::Wait(const int give_up_after_ms, int /*warn_after_ms*/) {
  const DWORD ms = give_up_after_ms == kForever ? INFINITE : give_up_after_ms;
  return (WaitForSingleObject(event_handle_, ms) == WAIT_OBJECT_0);
}
}  // namespace rtc
