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

#include <sole/sole.hpp>

#include <chrono>

#include "communicator.hpp"

#if defined(QNX)
#include <sys/neutrino.h>
#include <inttypes.h>
#include <sys/syspage.h>
#endif

namespace performance_test
{

Communicator::Communicator(SpinLock & lock, EventLogger & event_logger)
: m_ec(ExperimentConfiguration::get()),
  m_prev_timestamp(),
  m_event_logger(event_logger),
  m_pub_id(sole::uuid4().str()),
  m_sub_id(sole::uuid4().str()),
  m_prev_sample_id(),
  m_num_lost_samples(),
  m_received_sample_counter(),
  m_sent_sample_counter(),
  m_lock(lock)
{
#if defined(QNX)
  m_cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
#endif
}

std::uint64_t Communicator::num_received_samples() const
{
  return m_received_sample_counter;
}
std::uint64_t Communicator::num_sent_samples() const
{
  return m_sent_sample_counter;
}
std::uint64_t Communicator::num_lost_samples() const
{
  return m_num_lost_samples;
}
std::uint64_t Communicator::next_sample_id()
{
  /* We never send a sample with id 0 to make sure we not just dealing with default
     constructed samples.
   */
  m_prev_sample_id = m_prev_sample_id + 1;
  return m_prev_sample_id;
}
void Communicator::increment_received(const std::uint64_t & increment)
{
  m_received_sample_counter += increment;
}
void Communicator::increment_sent(const std::uint64_t & increment)
{
  m_sent_sample_counter += increment;
}
void Communicator::update_lost_samples_counter(const std::uint64_t sample_id)
{
  // We can lose samples, but samples always arrive in the right order and no duplicates exist.
  if (sample_id <= m_prev_sample_id) {
    throw std::runtime_error(
            "Data consistency violated. Received sample with not strictly higher "
            "id. Received sample id " +
            std::to_string(
              sample_id) + " Prev. sample id : " +
            std::to_string(m_prev_sample_id));
  }
  m_num_lost_samples += sample_id - m_prev_sample_id - 1;
  m_prev_sample_id = sample_id;
}
void Communicator::add_latency_to_statistics(const std::int64_t sample_timestamp)
{
#if defined(QNX)
  // Since clock resolution on QNX is 1ms or above, it is better to use clock cycles to
  // calculate latencies instead of CLOCK_REALTIME or CLOCK_MONOTONIC.
  std::uint64_t rclk_cyc = ClockCycles();
  std::uint64_t sclk_cyc = static_cast<std::uint64_t>(sample_timestamp);
  std::uint64_t ncycles = (rclk_cyc - sclk_cyc);
  // std::uint64_t m_cpms = (m_cps/1000);
  // std::uint64_t m_cpus = (m_cpms/1000);
  const double sec_diff = static_cast<double>(ncycles) / static_cast<double>(m_cps);
#else
  std::chrono::nanoseconds st(sample_timestamp);
  const auto diff = std::chrono::steady_clock::now().time_since_epoch() - st;

  const auto sec_diff = std::chrono::duration_cast<std::chrono::duration<double>>(diff).count();
#endif
  // Converting to double for easier calculations. Because the two timestamps are very close
  // double precision is enough.
  m_latency.add_sample(sec_diff);
}
StatisticsTracker Communicator::latency_statistics() const
{
  return m_latency;
}
void Communicator::reset()
{
  m_num_lost_samples = 0;
  m_received_sample_counter = 0;
  m_sent_sample_counter = 0;
  m_latency = StatisticsTracker();
}

std::uint64_t Communicator::prev_sample_id() const
{
  return m_prev_sample_id;
}

void Communicator::lock()
{
  m_lock.lock();
}

void Communicator::unlock()
{
  m_lock.unlock();
}

}  // namespace performance_test
