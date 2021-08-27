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

#ifndef EVENTS__EVENT_AGGREGATOR_HPP_
#define EVENTS__EVENT_AGGREGATOR_HPP_

#include <sys/resource.h>

#include <vector>
#include <memory>
#include <tuple>
#include <map>
#include <mutex>

#include "../events/event_sink.hpp"
#include "../utilities/cpu_info.hpp"
#include "../outputs/output.hpp"
#include "../utilities/statistics_tracker.hpp"
#include "../utilities/perf_clock.hpp"

namespace performance_test
{
class EventAggregator : public EventSink
{
public:
  EventAggregator(const std::vector<std::shared_ptr<Output>> & outputs);
  ~EventAggregator();
  void begin_transaction();
  void end_transaction();
  void register_pub(const EventRegisterPub & event);
  void register_sub(const EventRegisterSub & event);
  void message_sent(const EventMessageSent & event);
  void message_received(const EventMessageReceived & event);
  void system_measured(const EventSystemMeasured & event);

private:
  typedef std::uint64_t sequence_id;
  typedef std::int64_t timestamp;
  typedef std::string pub_id;
  typedef std::string sub_id;

  const PerfClock::time_point m_experiment_start_time;
  StatisticsTracker m_latency_statistics;
  uint64_t m_num_sent_samples;
  uint64_t m_num_received_samples;
  uint64_t m_num_lost_samples;
  std::size_t m_sum_data_received;

  // TODO(erik.snider) this design does not handle more than 1 pub,
  // or more than one topic. I think that changinge the keys to a
  // std::pair<pub_id, seq_id> would be the next step.
  // For full flexibility, the pub_id would also have to be embedded
  // in the message.
  std::map<sequence_id, timestamp> m_published_timestamps;
  std::map<sequence_id, int> m_received_count;
  std::map<sub_id, std::size_t> m_data_sizes;
  std::map<sub_id, sequence_id> m_latest_received;

  int m_num_subs;
  
  EventSystemMeasured m_event_system_measured;

  const std::vector<std::shared_ptr<Output>> m_outputs;

  bool m_run;
  std::thread m_thread;
  std::mutex m_mutex;

  void thread_function();
};
}  // namespace performance_test

#endif  // EVENTS__EVENT_AGGREGATOR_HPP_
