// Copyright 2017 Apex.AI, Inc.
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

#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

#include "analyze_runner.hpp"
#include "analysis_result.hpp"
#include "../events/event_db.hpp"
#include "../events/event_aggregator.hpp"
#include "../utilities/perf_clock.hpp"

namespace performance_test
{

AnalyzeRunner::AnalyzeRunner()
: m_ec(ExperimentConfiguration::get()),
  m_is_first_entry(true),
  m_event_logger(ExperimentConfiguration::get())
{
  for (uint32_t i = 0; i < m_ec.number_of_publishers(); ++i) {
    m_pub_runners.push_back(
      DataRunnerFactory::get(m_ec.msg_name(), m_ec.com_mean(), RunType::PUBLISHER, m_event_logger));
  }
  for (uint32_t i = 0; i < m_ec.number_of_subscribers(); ++i) {
    m_sub_runners.push_back(
      DataRunnerFactory::get(m_ec.msg_name(), m_ec.com_mean(), RunType::SUBSCRIBER, m_event_logger));
  }
}

void AnalyzeRunner::run()
{
  const auto experiment_start = PerfClock::now();

  while (!check_exit(experiment_start)) {
    measure_system();
    sleep(1);
#if PERFORMANCE_TEST_RT_ENABLED
    /// If there are custom RT settings and this is the first loop, set the post
    /// RT init settings
    if (m_is_first_entry && m_ec.is_rt_init_required()) {
      post_proc_rt_init();
      m_is_first_entry = false;
    }
#endif
  }
}

bool AnalyzeRunner::check_exit(PerfClock::time_point experiment_start) const
{
  if (m_ec.exit_requested()) {
    std::cout << "Caught signal. Exiting." << std::endl;
    return true;
  }

  if (m_ec.max_runtime() == 0) {
    // Run forever,
    return false;
  }

  const double runtime_sec =
    std::chrono::duration<double>(PerfClock::now() - experiment_start).count();

  if (runtime_sec > static_cast<double>(m_ec.max_runtime())) {
    std::cout << "Maximum runtime reached. Exiting." << std::endl;
    return true;
  } else {
    return false;
  }
}

void AnalyzeRunner::measure_system() {
  rusage sys_usage;
  const auto ret = getrusage(RUSAGE_SELF, &sys_usage);
#if defined(QNX)
  // QNX getrusage() max_rss does not give the correct value. Using a different method to get
  // the RSS value and converting into KBytes
  sys_usage.ru_maxrss =
    (static_cast<int64_t>(performance_test::qnx_res::get_proc_rss_mem()) / 1024);
#endif
  if (ret != 0) {
    throw std::runtime_error("Could not get system resource usage.");
  }

  m_event_logger.system_measured(
    cpu_usage_tracker.get_cpu_usage(), sys_usage, PerfClock::timestamp());
}

}  // namespace performance_test
