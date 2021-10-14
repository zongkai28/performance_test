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

#include "event_live_printer.hpp"
#include "../experiment_configuration/experiment_configuration.hpp"

#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <functional>

namespace performance_test
{

EventLivePrinter::EventLivePrinter(
  const std::shared_ptr<EventSource> & event_source,
  const std::string & topic,
  const std::vector<std::shared_ptr<Output>> & outputs)
: m_event_source(event_source),
  m_topic(topic),
  m_outputs(outputs),
  m_experiment_start_time(PerfClock::now()),
  m_run(true),
  m_thread(std::bind(&EventLivePrinter::thread_function, this))
{
  for (const auto & output : m_outputs) {
    output->open();
  }
}

EventLivePrinter::~EventLivePrinter() {
  m_run = false;
  m_thread.join();
  for (const auto & output : m_outputs) {
    output->close();
  }
}

void EventLivePrinter::thread_function()
{
  typedef std::uint64_t sequence_id;
  typedef std::int64_t timestamp;
  typedef std::string pub_id;
  typedef std::string sub_id;
  typedef std::pair<pub_id, sequence_id> message_id;

  {
    auto n = ExperimentConfiguration::get().rows_to_ignore();
    std::this_thread::sleep_for(std::chrono::seconds{n});
  }

  const auto pubs = m_event_source->query_register_pub(m_topic);
  const auto subs = m_event_source->query_register_sub(m_topic);

  const auto n_pubs = pubs.size();
  const auto n_subs = subs.size();
  const std::size_t data_size = (n_pubs > 0) ? subs[0].data_size : 0;

  std::map<message_id, timestamp> published_timestamps;
  std::map<message_id, int> received_count;
  std::map<sub_id, std::map<pub_id, sequence_id>> latest_received;
  
  PerfClock::time_point loop_start;
  PerfClock::time_point loop_end = PerfClock::now();
  while (m_run) {
    loop_start = loop_end;
    loop_end += std::chrono::seconds{1};
    std::this_thread::sleep_until(loop_end);

    StatisticsTracker latency_statistics;
    uint64_t num_lost_samples = 0;

    const auto msg_sent = m_event_source->query_message_sent(loop_start, loop_end, m_topic);
    const auto msg_rcvd = m_event_source->query_message_received(loop_start, loop_end, m_topic);
    const auto sys_mmt = m_event_source->query_system_measured(loop_start, loop_end);

    for (const auto event : msg_sent) {
      const message_id msg_id = std::make_pair(event.pub_id, event.sequence_id);
      published_timestamps[msg_id] = event.timestamp;
    }
    
    for (const auto event : msg_rcvd) {
      const sequence_id seq_id = event.sequence_id;
      const message_id msg_id = std::make_pair(event.pub_id, seq_id);
      
      if (published_timestamps.count(msg_id) == 0) {
        throw std::runtime_error("Received a message that wasn't published!");
      }
       
      const auto diff_count = event.timestamp - published_timestamps[msg_id];
      const auto diff_ns = std::chrono::nanoseconds{diff_count};
      const auto sec_diff = std::chrono::duration_cast<std::chrono::duration<double>>(diff_ns).count();
      latency_statistics.add_sample(sec_diff);

      sequence_id prev = latest_received[event.sub_id][event.pub_id];

      // Ensure samples always arrive in the right order and no duplicates exist
      if (seq_id <= prev) {
        throw std::runtime_error(
          "Data consistency violated. Received sample with not strictly higher id."
          " Received sample id " + std::to_string(seq_id) +
          " Prev. sample id : " + std::to_string(prev));
      }

      for (sequence_id i = prev + 1; i < seq_id; i++) {
        num_lost_samples++;
        auto lost_msg_id = std::make_pair(event.pub_id, i);
        received_count[lost_msg_id]++;
        if (received_count[lost_msg_id] == n_subs) {
          published_timestamps.erase(lost_msg_id);
          received_count.erase(lost_msg_id);
        }
      }

      received_count[msg_id]++;
      if (received_count[msg_id] == n_subs) {
        published_timestamps.erase(msg_id);
        received_count.erase(msg_id);
      }

      latest_received[event.sub_id][event.pub_id] = seq_id;
    }

    uint64_t num_sent_samples = msg_sent.size();
    uint64_t num_received_samples = msg_rcvd.size();
    std::size_t sum_data_received = num_received_samples * data_size;

    const EventSystemMeasured & event_system_measured = sys_mmt.back();

    auto result = std::make_shared<const AnalysisResult>(
      std::chrono::nanoseconds{loop_end - m_experiment_start_time},
      std::chrono::nanoseconds{loop_end - loop_start},
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
