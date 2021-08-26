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

#include "event_logger.hpp"

#include <string>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <functional>

namespace performance_test
{

EventLogger::EventLogger(const std::vector<std::shared_ptr<EventSink>> & event_sinks)
: m_event_sinks(event_sinks),
  m_run(true),
  m_thread(std::bind(&EventLogger::thread_function, this))
{
}

EventLogger::~EventLogger()
{
  m_run = false;
  m_thread.join();
}

void EventLogger::register_pub(
  const std::string & pub_id, const std::string & msg_type, const std::string & topic)
{
  m_q_register_pub.enqueue(std::forward_as_tuple(pub_id, msg_type, topic));
}

void EventLogger::register_sub(
  const std::string & sub_id, const std::string & msg_type, const std::string & topic)
{
  m_q_register_sub.enqueue(std::forward_as_tuple(sub_id, msg_type, topic));
}

void EventLogger::message_sent(
  const std::string & pub_id, std::uint64_t sequence_id, std::int64_t timestamp)
{
  m_q_message_sent.enqueue(std::forward_as_tuple(pub_id, sequence_id, timestamp));
}

void EventLogger::message_received(
  const std::string & sub_id, std::uint64_t sequence_id, std::int64_t timestamp)
{
  m_q_message_received.enqueue(std::forward_as_tuple(sub_id, sequence_id, timestamp));
}

void EventLogger::system_measured(
  const CpuInfo & cpu_info, const rusage & sys_usage, std::int64_t timestamp)
{
  m_q_system_measured.enqueue(std::forward_as_tuple(cpu_info, sys_usage, timestamp));
}

void EventLogger::thread_function()
{
  while (m_run) {
    for (auto & sink : m_event_sinks) {
      sink->begin_transaction();
    }

    event_register_pub erp;
    while(m_q_register_pub.try_dequeue(erp)) {
      for (auto & sink : m_event_sinks) {
        sink->register_pub(std::get<0>(erp), std::get<1>(erp), std::get<2>(erp));
      }
    }

    event_register_pub ers;
    while(m_q_register_sub.try_dequeue(ers)) {
      for (auto & sink : m_event_sinks) {
        sink->register_sub(std::get<0>(ers), std::get<1>(ers), std::get<2>(ers));
      }
    }

    event_message_sent ems;
    while(m_q_message_sent.try_dequeue(ems)) {
      for (auto & sink : m_event_sinks) {
        sink->message_sent(std::get<0>(ems), std::get<1>(ems), std::get<2>(ems));
      }
    }

    event_message_sent emr;
    while(m_q_message_received.try_dequeue(emr)) {
      for (auto & sink : m_event_sinks) {
        sink->message_received(std::get<0>(emr), std::get<1>(emr), std::get<2>(emr));
      }
    }

    event_system_measured esm;
    while(m_q_system_measured.try_dequeue(esm)) {
      for (auto & sink : m_event_sinks) {
        sink->system_measured(std::get<0>(esm), std::get<1>(esm), std::get<2>(esm));
      }
    }

    for (auto & sink : m_event_sinks) {
      sink->end_transaction();
    }
  }
}

}  // namespace performance_test
