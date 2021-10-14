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

#ifndef EVENTS__EVENT_LIVE_PRINTER_HPP_
#define EVENTS__EVENT_LIVE_PRINTER_HPP_

#include <sys/resource.h>

#include <vector>
#include <memory>
#include <tuple>
#include <map>
#include <mutex>

#include "event_source.hpp"
#include "../outputs/output.hpp"
#include "../utilities/perf_clock.hpp"

namespace performance_test
{
class EventLivePrinter
{
public:
  EventLivePrinter(
    const std::shared_ptr<EventSource> & event_source,
    const std::string & topic,
    const std::vector<std::shared_ptr<Output>> & outputs);
  ~EventLivePrinter();
  
private:
  const std::shared_ptr<EventSource> m_event_source;
  const std::string m_topic;
  const std::vector<std::shared_ptr<Output>> m_outputs;

  const PerfClock::time_point m_experiment_start_time;

  bool m_run;
  std::thread m_thread;

  void thread_function();
};
}  // namespace performance_test

#endif  // EVENTS__EVENT_LIVE_PRINTER_HPP_
