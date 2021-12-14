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

#ifndef COMMUNICATION_ABSTRACTIONS__COMMUNICATOR_HPP_
#define COMMUNICATION_ABSTRACTIONS__COMMUNICATOR_HPP_

#include <stdexcept>
#include <limits>
#include <atomic>

#include "../utilities/spin_lock.hpp"
#include "../utilities/statistics_tracker.hpp"
#include "../experiment_configuration/experiment_configuration.hpp"

namespace performance_test
{

/// Helper base class for communication plugins which provides sample tracking helper functionality.
class Communicator
{
public:
  /// Constructor which takes a reference \param lock to the lock to use.
  explicit Communicator(SpinLock & lock);

  /// Number of received samples.
  std::uint64_t num_received_samples() const;
  /// Number of sent samples.
  std::uint64_t num_sent_samples() const;
  /// Number of lost samples.
  std::uint64_t num_lost_samples() const;
  /**
   * \brief Adds a sample timestamp to the latency statistics.
   * \param sample_timestamp The timestamp the sample was sent.
   */
  void add_latency_to_statistics(const std::int64_t sample_timestamp);
  /// Returns stored latency statistics.
  StatisticsTracker latency_statistics() const;
  /// Resets all internal counters.
  void reset();

protected:
  /// Get the the id for the next sample to publish.
  std::uint64_t next_sample_id();
  /**
   * \brief Increment the number of received samples.
   * \param increment Optional different increment step.
   */
  void increment_received(const std::uint64_t & increment = 1);
  /**
   * \brief Increment the number of sent samples.
   * \param increment Optional different increment step.
   */
  void increment_sent(const std::uint64_t & increment = 1);
  /**
   * \brief Given a sample id this function check if and how many samples were lost and
   *        updates counters accordingly.
   * \param sample_id The sample id to check.
   */
  void update_lost_samples_counter(const std::uint64_t sample_id);

  /// The experiment configuration.
  const ExperimentConfiguration & m_ec;
  /// The time the last sample was received [ns since epoc].
  std::int64_t m_prev_timestamp;

  /// Lock the spinlock.
  void lock();
  /// Unlock the spinlock.
  void unlock();

  /// Returns the lock.
  auto & get_lock()
  {
    return m_lock;
  }

  // TODO(erik.snider) switch to std::void_t when upgrading to C++17
  template<class ...>
  using void_t = void;

  template<typename T, typename = void>
  struct has_bounded_sequence : std::false_type {};

  template<typename T>
  struct has_bounded_sequence<T,
    void_t<decltype(std::declval<T>().bounded_sequence)>>: std::true_type {};

  template<typename T, typename = void>
  struct has_unbounded_sequence : std::false_type {};

  template<typename T>
  struct has_unbounded_sequence<T,
    void_t<decltype(std::declval<T>().unbounded_sequence)>>: std::true_type {};

  template<typename T, typename = void>
  struct has_unbounded_string : std::false_type {};

  template<typename T>
  struct has_unbounded_string<T,
    void_t<decltype(std::declval<T>().unbounded_string)>>: std::true_type {};

  template<typename T>
  inline
  void init_msg(T & msg, std::int64_t time)
  {
    msg.time = time;
    msg.id = next_sample_id();
    ensure_fixed_size(msg);
  }

  template<typename T>
  inline
  std::enable_if_t<
    has_bounded_sequence<T>::value ||
    has_unbounded_sequence<T>::value ||
    has_unbounded_string<T>::value, void>
  ensure_fixed_size(T &)
  {
    throw std::runtime_error("This plugin only supports messages with a fixed size");
  }

  template<typename T>
  inline
  std::enable_if_t<
    !has_bounded_sequence<T>::value &&
    !has_unbounded_sequence<T>::value &&
    !has_unbounded_string<T>::value, void>
  ensure_fixed_size(T &) {}

private:
  std::uint64_t m_prev_sent_sample_id;
  std::uint64_t m_prev_received_sample_id;
  std::uint64_t m_num_lost_samples;
  std::uint64_t m_received_sample_counter;
  std::uint64_t m_sent_sample_counter;
#if defined(QNX)
  std::uint64_t m_cps;
#endif

  StatisticsTracker m_latency;

  SpinLock & m_lock;
};

}  // namespace performance_test

#endif  // COMMUNICATION_ABSTRACTIONS__COMMUNICATOR_HPP_
