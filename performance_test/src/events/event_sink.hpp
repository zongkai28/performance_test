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

#ifndef EVENTS__EVENT_SINK_HPP_
#define EVENTS__EVENT_SINK_HPP_

#include "event_objects.hpp"

namespace performance_test
{
class EventSink
{
public:
  EventSink() = default;
  virtual ~EventSink() = default;

  virtual void begin_transaction() = 0;
  virtual void end_transaction() = 0;
  
  virtual void register_pub(const EventRegisterPub & event) = 0;
  virtual void register_sub(const EventRegisterSub & event) = 0;
  virtual void message_sent(const EventMessageSent & event) = 0;
  virtual void message_received(const EventMessageReceived & event) = 0;
  virtual void system_measured(const EventSystemMeasured & event) = 0;
};
}  // namespace performance_test

#endif  // EVENTS__EVENT_SINK_HPP_
