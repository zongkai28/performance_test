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
: m_experiment_start_time(PerfClock::now()),
  m_num_sent_samples(0),
  m_num_received_samples(0),
  m_num_lost_samples(0),
  m_sum_data_received(0),
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

void EventAggregator::register_sub(const EventRegisterSub & event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_num_subs++;
  m_data_sizes[event.sub_id] = event.data_size;
}

void EventAggregator::message_sent(const EventMessageSent & event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_num_sent_samples++;
  m_published_timestamps[event.sequence_id] = event.timestamp;
}

void EventAggregator::message_received(const EventMessageReceived & event)
{
  sequence_id seq_id = event.sequence_id;
  std::lock_guard<std::mutex> guard(m_mutex);
  m_num_received_samples++;

  // When the system gets busy, sometimes the events don't all show up
  // exactly in order, even if the samples all were sent and received in order.
  // Maybe cache stuff in begin/end transaction to address this?
  if (m_published_timestamps.count(seq_id) != 0) {
    auto diff_count = event.timestamp - m_published_timestamps[seq_id];
    auto diff_ns = std::chrono::nanoseconds{diff_count};
    auto sec_diff = std::chrono::duration_cast<std::chrono::duration<double>>(diff_ns).count();
    m_latency_statistics.add_sample(sec_diff);
  }

  m_received_count[seq_id]++;
  if (m_received_count[seq_id] == m_num_subs) {
    m_published_timestamps.erase(seq_id);
    m_received_count.erase(seq_id);
  }

  sequence_id prev = m_latest_received[event.sub_id];

  // Ensure samples always arrive in the right order and no duplicates exist
  if (seq_id <= prev) {
    throw std::runtime_error(
      "Data consistency violated. Received sample with not strictly higher id."
      " Received sample id " + std::to_string(seq_id) +
      " Prev. sample id : " + std::to_string(prev));
  }

  m_num_lost_samples += (seq_id - prev - 1);
  m_latest_received[event.sub_id] = seq_id;

  m_sum_data_received += m_data_sizes[event.sub_id];
}

void EventAggregator::system_measured(const EventSystemMeasured & event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_event_system_measured = event;
}

void EventAggregator::thread_function()
{
  StatisticsTracker latency_statistics;
  uint64_t num_sent_samples;
  uint64_t num_received_samples;
  uint64_t num_lost_samples;
  std::size_t sum_data_received;
  EventSystemMeasured event_system_measured;

  auto report_time = m_experiment_start_time;
  while (m_run) {
    const auto loop_start = PerfClock::now();
    report_time += std::chrono::seconds{1};
    std::this_thread::sleep_until(report_time);

    {
      std::lock_guard<std::mutex> guard(m_mutex);

      latency_statistics = m_latency_statistics;
      m_latency_statistics = StatisticsTracker();

      num_sent_samples = m_num_sent_samples;
      m_num_sent_samples = 0;

      num_received_samples = m_num_received_samples;
      m_num_received_samples = 0;

      num_lost_samples = m_num_lost_samples;
      m_num_lost_samples = 0;

      sum_data_received = m_sum_data_received;
      m_sum_data_received = 0;

      event_system_measured = m_event_system_measured;
    }

    auto now = PerfClock::now();

    auto result = std::make_shared<const AnalysisResult>(
      std::chrono::nanoseconds{now - m_experiment_start_time},
      std::chrono::nanoseconds{now - loop_start},
      num_received_samples,
      num_sent_samples,
      num_lost_samples,
      sum_data_received,
      latency_statistics,
      StatisticsTracker(),  // residuals
      StatisticsTracker(),
      event_system_measured.cpu_info,
      event_system_measured.sys_usage
    );

    for (const auto & output : m_outputs) {
      output->update(result);
    }
  }
}

}  // namespace performance_test
