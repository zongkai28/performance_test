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

#include "event_aggregator.hpp"
#include "event_db.hpp"
#include "event_aggregator.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <functional>

namespace performance_test
{

EventAggregator::EventAggregator(const std::vector<std::shared_ptr<Output>> & outputs)
: m_experiment_start_time(perf_clock::now()),
  m_outputs(outputs),
  m_run(true),
  m_thread(std::bind(&EventAggregator::thread_function, this))
{
  for (const auto & output : m_outputs) {
    output->open();
  }
}

EventAggregator::~EventAggregator() {
  m_run = false;
  m_thread.join();
  for (const auto & output : m_outputs) {
    output->close();
  }
}

void EventAggregator::begin_transaction()
{
}

void EventAggregator::end_transaction()
{
}

void EventAggregator::register_pub(const EventRegisterPub &)
{
}

void EventAggregator::register_sub(const EventRegisterSub &)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_num_subs++;
}

void EventAggregator::message_sent(const EventMessageSent & event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_published_timestamps[event.sequence_id] = event.timestamp;
}

void EventAggregator::message_received(const EventMessageReceived & event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  sequence_id id = event.sequence_id;
  std::chrono::nanoseconds ts_sent(m_published_timestamps[id]);
  std::chrono::nanoseconds ts_rcvd(event.timestamp);
  auto diff = ts_rcvd - ts_sent;
  auto sec_diff = std::chrono::duration_cast<std::chrono::duration<double>>(diff).count();
  m_latency_statistics.add_sample(sec_diff);

  m_received_count[id]++;
  if (m_received_count[id] == m_num_subs) {
    m_published_timestamps.erase(id);
    m_received_count.erase(id);
  }
}

void EventAggregator::system_measured(const EventSystemMeasured & event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_event_system_measured = event;
}

void EventAggregator::thread_function()
{
  StatisticsTracker latency_statistics;
  EventSystemMeasured event_system_measured;

  auto report_time = m_experiment_start_time;
  while (m_run) {
    const auto loop_start = perf_clock::now();
    report_time += std::chrono::seconds{1};
    std::this_thread::sleep_until(report_time);

    {
      std::lock_guard<std::mutex> guard(m_mutex);

      latency_statistics = m_latency_statistics;
      m_latency_statistics = StatisticsTracker();

      event_system_measured = m_event_system_measured;
    }

    auto now = perf_clock::now();

    auto result = std::make_shared<const AnalysisResult>(
      std::chrono::nanoseconds{now - m_experiment_start_time},
      std::chrono::nanoseconds{now - loop_start},
      0, //sum_received_samples,
      0, //sum_sent_samples,
      0, //sum_lost_samples,
      0, //sum_data_received,
      latency_statistics,
      StatisticsTracker(),
      StatisticsTracker(),
      event_system_measured.cpu_info
    );

    for (const auto & output : m_outputs) {
      output->update(result);
    }
  }
}

}  // namespace performance_test
