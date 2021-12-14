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
#include <functional>

#if defined(QNX)
#include <sys/neutrino.h>
#include <inttypes.h>
#endif

#include "../utilities/spin_lock.hpp"

#ifdef QNX710
using perf_clock = std::chrono::system_clock;
#else
using perf_clock = std::chrono::steady_clock;
#endif

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
  explicit DataRunner(const RunType run_type)
  : m_com(m_lock),
    m_run(true),
    m_sum_received_samples(0),
    m_sum_lost_samples(0),
    m_sum_received_data(0),
    m_sum_sent_samples(0),
    m_last_sync(perf_clock::now()),
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

  uint64_t sum_received_samples() const override
  {
    if (!is_subscriber()) {
      throw std::logic_error("Only available on a subscriber.");
    }
    return m_sum_received_samples;
  }
  uint64_t sum_lost_samples() const override
  {
    if (!is_subscriber()) {
      throw std::logic_error("Only available on a subscriber.");
    }
    return m_sum_lost_samples;
  }
  uint64_t sum_sent_samples() const override
  {
    if (!is_publisher()) {
      throw std::logic_error("Only available on a publisher.");
    }
    return m_sum_sent_samples;
  }
  std::size_t sum_data_received() const override
  {
    if (!is_subscriber()) {
      throw std::logic_error("Only available on a subscriber.");
    }
    return m_sum_received_data;
  }
  StatisticsTracker latency_statistics() const override
  {
    if (!is_subscriber()) {
      throw std::logic_error("Only available on a subscriber.");
    }
    return m_latency_statistics;
  }
  StatisticsTracker loop_time_reserve_statistics() const override
  {
    return m_time_reserve_statistics_store;
  }
  void sync_reset() override
  {
    namespace sc = std::chrono;
    const auto now = perf_clock::now();
    m_lock.lock();
    sc::duration<double> iteration_duration = now - m_last_sync;

    if (is_publisher()) {
      m_sum_sent_samples = static_cast<decltype(m_sum_sent_samples)>(
        static_cast<double>(m_com.num_sent_samples()) / iteration_duration.count());
      m_sum_lost_samples = static_cast<decltype(m_sum_lost_samples)>(
        static_cast<double>(m_com.num_lost_samples()) / iteration_duration.count());
    }
    if (is_subscriber()) {
      m_sum_received_samples = static_cast<decltype(m_sum_received_samples)>(
        static_cast<double>(m_com.num_received_samples()) / iteration_duration.count());
      m_sum_received_data = static_cast<decltype(m_sum_received_data)>(
        static_cast<double>(m_com.data_received()) / iteration_duration.count());
      m_sum_lost_samples = static_cast<decltype(m_sum_lost_samples)>(
        static_cast<double>(m_com.num_lost_samples()) / iteration_duration.count());
      m_latency_statistics = m_com.latency_statistics();
    }
    m_time_reserve_statistics_store = m_time_reserve_statistics;
    m_time_reserve_statistics = StatisticsTracker();
    m_com.reset();
    m_last_sync = now;

    m_lock.unlock();
  }

private:
  bool is_publisher() const
  {
    return m_run_type == RunType::PUBLISHER || m_run_type == RunType::BOTH;
  }

  bool is_subscriber() const
  {
    return m_run_type == RunType::SUBSCRIBER || m_run_type == RunType::BOTH;
  }

  /// The function running inside the thread doing all the work.
  void thread_function()
  {
    auto next_run = perf_clock::now() +
      std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double>(1.0 / m_ec.rate()));

    auto first_run = perf_clock::now();

    std::size_t loop_counter = 1;

    while (m_run) {
      if (is_publisher() &&
        m_ec.roundtrip_mode() != ExperimentConfiguration::RoundTripMode::RELAY)
      {
#if defined(QNX)
        std::int64_t epoc_time = static_cast<std::int64_t>(ClockCycles());
#else
        std::int64_t epoc_time =
          perf_clock::now().time_since_epoch().count();
#endif
        m_com.publish(epoc_time);
      }
      if (is_subscriber()) {
        m_com.update_subscription();
      }
      const std::chrono::nanoseconds reserve = next_run - perf_clock::now();
      {
        // We track here how much time (can also be negative) was left for the loop iteration given
        // the desired loop rate.
        m_lock.lock();
        if (is_publisher()) {
          m_time_reserve_statistics.add_sample(std::chrono::duration<double>(reserve).count());
        } else {
          m_time_reserve_statistics.add_sample(0.0);
        }
        m_lock.unlock();
      }
      if (m_ec.rate() > 0 &&
        is_publisher() &&
        // Relays should never sleep.
        m_ec.roundtrip_mode() != ExperimentConfiguration::RoundTripMode::RELAY)
      {
        if (reserve.count() > 0) {
          std::this_thread::sleep_until(next_run);
        }
      }
      next_run = first_run + loop_counter *
        std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(1.0 / static_cast<double>(m_ec.rate())));

      ++loop_counter;
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
  SpinLock m_lock;

  uint64_t m_sum_received_samples;
  uint64_t m_sum_lost_samples;
  std::size_t m_sum_received_data;

  std::uint64_t m_sum_sent_samples;

  StatisticsTracker m_latency_statistics;
  StatisticsTracker m_time_reserve_statistics, m_time_reserve_statistics_store;

  perf_clock::time_point m_last_sync;
  const RunType m_run_type;

  std::thread m_thread;

  bool m_memory_tools_on = false;
};

}  // namespace performance_test

#endif  // DATA_RUNNING__DATA_RUNNER_HPP_
