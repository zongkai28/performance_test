// Copyright 2021 Apex.AI, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef EVENTS__EVENT_SOURCE_HPP_
#define EVENTS__EVENT_SOURCE_HPP_

#include <vector>
#include <string>

#include "event_objects.hpp"
#include "../utilities/perf_clock.hpp"

namespace performance_test
{
class EventSource
{
public:
  EventSource() = default;
  virtual ~EventSource() = default;

  virtual std::vector<EventRegisterPub> query_register_pub(
    const std::string & topic) = 0;

  virtual std::vector<EventRegisterSub> query_register_sub(
    const std::string & topic) = 0;

  virtual std::vector<EventMessageSent> query_message_sent(
    PerfClock::time_point start,
    PerfClock::time_point end,
    const std::string & topic) = 0;

  virtual std::vector<EventMessageReceived> query_message_received(
    PerfClock::time_point start,
    PerfClock::time_point end,
    const std::string & topic) = 0;

  virtual std::vector<EventSystemMeasured> query_system_measured(
    PerfClock::time_point start,
    PerfClock::time_point end) = 0;
};
}  // namespace performance_test

#endif  // EVENTS__EVENT_SOURCE_HPP_
