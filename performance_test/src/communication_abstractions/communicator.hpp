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

#include <sole/sole.hpp>

#include <stdexcept>
#include <limits>
#include <atomic>
#include <string>

#include "../utilities/spin_lock.hpp"
#include "../utilities/statistics_tracker.hpp"
#include "../experiment_configuration/experiment_configuration.hpp"
#include "../events/event_logger.hpp"
#include "../utilities/perf_clock.hpp"

namespace performance_test
{

/// Helper base class for communication plugins which provides sample tracking helper functionality.
class Communicator
{
public:
  /// Constructor which takes a reference \param lock to the lock to use.
  explicit Communicator(EventLogger & event_logger);

protected:
  /// Get the the id for the next sample to publish.
  std::uint64_t next_sample_id();

  /// The experiment configuration.
  const ExperimentConfiguration & m_ec;

  EventLogger & m_event_logger;
  const std::string m_pub_id;
  const std::string m_sub_id;

private:
  std::uint64_t m_prev_sample_id;
};

}  // namespace performance_test

#endif  // COMMUNICATION_ABSTRACTIONS__COMMUNICATOR_HPP_
