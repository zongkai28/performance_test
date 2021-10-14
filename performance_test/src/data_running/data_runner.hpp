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

#ifndef DATA_RUNNING__DATA_RUNNER_HPP_
#define DATA_RUNNING__DATA_RUNNER_HPP_

#include "data_runner_base.hpp"
#ifdef PERFORMANCE_TEST_MEMORYTOOLS_ENABLED
#include <osrf_testing_tools_cpp/memory_tools/memory_tools.hpp>
#endif
#include <atomic>
#include <memory>
#include <thread>

#include "../utilities/perf_clock.hpp"
#include "../events/event_logger.hpp"

namespace performance_test
{

/// Generic class which effectively executes the experiment and collects the experiment results.
template<class TCommunicator>
class DataRunner : public DataRunnerBase
{
public:
  /**
   * \brief Constructs an object and starts the internal worker thread.
   * \param run_type Specifies which type of operation to execute.
   */
  DataRunner(const RunType run_type, EventLogger & event_logger)
  : m_com(event_logger),
    m_run(true),
    m_run_type(run_type),
    m_thread(std::bind(&DataRunner::thread_function, this))
  {
  }

  DataRunner & operator=(const DataRunner &) = delete;
  DataRunner(const DataRunner &) = delete;

  ~DataRunner() noexcept override
  {
    m_run = false;
    m_thread.join();
  }

private:
  /// The function running inside the thread doing all the work.
  void thread_function()
  {
    const auto run_increment = 
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(1.0 / m_ec.rate()));
    auto next_run = PerfClock::now() + run_increment;

    while (m_run) {
      if (m_run_type == RunType::PUBLISHER) {
        m_com.publish();
      }
      if (m_run_type == RunType::SUBSCRIBER) {
        m_com.update_subscription();
      }

      if (m_ec.rate() > 0 && m_run_type == RunType::PUBLISHER) {
        std::this_thread::sleep_until(next_run);
      }
      next_run += run_increment;

      // Enabling memory checker after the first run:
      enable_memory_tools();
    }
  }

  /// Enables the memory tool checker.
  void enable_memory_tools()
  {
    #ifdef PERFORMANCE_TEST_MEMORYTOOLS_ENABLED
    // Do not turn the memory tools on several times.
    if (m_memory_tools_on) {
      return;
    }

    osrf_testing_tools_cpp::memory_tools::expect_no_calloc_begin();
    osrf_testing_tools_cpp::memory_tools::expect_no_free_begin();
    osrf_testing_tools_cpp::memory_tools::expect_no_malloc_begin();
    osrf_testing_tools_cpp::memory_tools::expect_no_realloc_begin();

    m_memory_tools_on = true;
    #endif
  }
  TCommunicator m_com;
  std::atomic<bool> m_run;

  StatisticsTracker m_time_reserve_statistics, m_time_reserve_statistics_store;

  PerfClock::time_point m_last_sync;
  const RunType m_run_type;

  std::thread m_thread;

  bool m_memory_tools_on = false;
};

}  // namespace performance_test

#endif  // DATA_RUNNING__DATA_RUNNER_HPP_
