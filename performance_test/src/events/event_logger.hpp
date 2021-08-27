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

#ifndef EVENTS__EVENT_LOGGER_HPP_
#define EVENTS__EVENT_LOGGER_HPP_

#include <sys/resource.h>

#include <string>
#include <thread>
#include <tuple>
#include <vector>
#include <memory>

#include "../utilities/cpu_info.hpp"
#include "../experiment_configuration/experiment_configuration.hpp"
#include "concurrentqueue.h"
#include "event_sink.hpp"

namespace performance_test
{
class EventLogger
{
public:
  EventLogger(const ExperimentConfiguration & ec);
  ~EventLogger();
  void register_pub(
    const std::string & pub_id, const std::string & msg_type, const std::string & topic);
  void register_sub(
    const std::string & sub_id,
    const std::string & msg_type,
    const std::string & topic,
    std::size_t data_size);
  void message_sent(
    const std::string & pub_id, std::uint64_t sequence_id, std::int64_t timestamp);
  void message_received(
    const std::string & sub_id, std::uint64_t sequence_id, std::int64_t timestamp);
  void system_measured(
    const CpuInfo & cpu_info, const rusage & sys_usage, std::int64_t timestamp);

private:
  moodycamel::ConcurrentQueue<EventRegisterPub> m_q_register_pub;
  moodycamel::ConcurrentQueue<EventRegisterSub> m_q_register_sub;
  moodycamel::ConcurrentQueue<EventMessageSent> m_q_message_sent;
  moodycamel::ConcurrentQueue<EventMessageReceived> m_q_message_received;
  moodycamel::ConcurrentQueue<EventSystemMeasured> m_q_system_measured;

  const std::vector<std::shared_ptr<EventSink>> m_event_sinks;

  bool m_run;
  std::thread m_thread;

  void thread_function();

  static std::vector<std::shared_ptr<EventSink>>
  create_event_sinks(const ExperimentConfiguration & ec);
};
}  // namespace performance_test

#endif  // EVENTS__EVENT_LOGGER_HPP_
