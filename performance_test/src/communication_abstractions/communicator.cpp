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

namespace performance_test
{

Communicator::Communicator(EventLogger & event_logger)
: m_ec(ExperimentConfiguration::get()),
  m_event_logger(event_logger),
  m_pub_id(sole::uuid4().str()),
  m_sub_id(sole::uuid4().str()),
  m_prev_sample_id(0)
{
}

std::uint64_t Communicator::next_sample_id()
{
  /* We never send a sample with id 0 to make sure we not just dealing with default
     constructed samples.
   */
  m_prev_sample_id = m_prev_sample_id + 1;
  return m_prev_sample_id;
}

}  // namespace performance_test
