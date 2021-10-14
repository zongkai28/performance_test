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
#include "event_db.hpp"
#include "event_aggregator.hpp"

#include <sole/sole.hpp>

#include <string>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <functional>

namespace performance_test
{

EventLogger::EventLogger(const ExperimentConfiguration & ec)
: m_event_sinks(create_event_sinks(ec)),
  m_run(true),
  m_thread(std::bind(&EventLogger::thread_function, this))
{
  std::cerr << "NUM_SINKS: " << m_event_sinks.size() << std::endl;
}

EventLogger::~EventLogger()
{
  m_run = false;
  m_thread.join();
}

void EventLogger::register_pub(
  const std::string & pub_id, const std::string & msg_type, const std::string & topic)
{
  m_q_register_pub.enqueue(EventRegisterPub{pub_id, msg_type, topic});
}

void EventLogger::register_sub(
  const std::string & sub_id,
  const std::string & msg_type,
  const std::string & topic,
  std::size_t data_size)
{
  m_q_register_sub.enqueue(EventRegisterSub{sub_id, msg_type, topic, data_size});
}

void EventLogger::message_sent(
  const std::string & pub_id, std::uint64_t sequence_id, std::int64_t timestamp)
{
  m_q_message_sent.enqueue(EventMessageSent{pub_id, sequence_id, timestamp});
}

void EventLogger::message_received(
  const std::string & sub_id,
  const std::string & pub_id,
  std::uint64_t sequence_id,
  std::int64_t timestamp)
{
  m_q_message_received.enqueue(EventMessageReceived{sub_id, pub_id, sequence_id, timestamp});
}

void EventLogger::system_measured(
  const CpuInfo & cpu_info, const rusage & sys_usage, std::int64_t timestamp)
{
  m_q_system_measured.enqueue(EventSystemMeasured{cpu_info, sys_usage, timestamp});
}

void EventLogger::thread_function()
{
  while (m_run) {
    for (auto & sink : m_event_sinks) {
      sink->begin_transaction();
    }

    EventRegisterPub erp;
    while(m_q_register_pub.try_dequeue(erp)) {
      for (auto & sink : m_event_sinks) {
        sink->register_pub(erp);
      }
    }

    EventRegisterSub ers;
    while(m_q_register_sub.try_dequeue(ers)) {
      for (auto & sink : m_event_sinks) {
        sink->register_sub(ers);
      }
    }

    EventMessageSent ems;
    while(m_q_message_sent.try_dequeue(ems)) {
      for (auto & sink : m_event_sinks) {
        sink->message_sent(ems);
      }
    }

    EventMessageReceived emr;
    while(m_q_message_received.try_dequeue(emr)) {
      for (auto & sink : m_event_sinks) {
        sink->message_received(emr);
      }
    }

    EventSystemMeasured esm;
    while(m_q_system_measured.try_dequeue(esm)) {
      for (auto & sink : m_event_sinks) {
        sink->system_measured(esm);
      }
    }

    for (auto & sink : m_event_sinks) {
      sink->end_transaction();
    }
  }
}

std::vector<std::shared_ptr<performance_test::EventSink>>
EventLogger::create_event_sinks(const ExperimentConfiguration & ec) {
  std::vector<std::shared_ptr<EventSink>> sinks;
  
  if (ec.output_event_db()) {
    sinks.push_back(std::make_shared<EventDB>(sole::uuid4().str() + ".db"));
  }
  
  // auto outputs = ec.configured_outputs();
  // if (!outputs.empty()) {
  //   sinks.push_back(std::make_shared<EventAggregator>(outputs));
  // }

  return sinks;
}

}  // namespace performance_test
