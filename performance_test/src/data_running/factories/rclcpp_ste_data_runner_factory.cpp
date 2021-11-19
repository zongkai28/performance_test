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

#include "rclcpp_ste_data_runner_factory.hpp"

#include <performance_test/generated_messages/messages.hpp>
#include <performance_test/for_each.hpp>

#include <string>
#include <memory>

#include "../data_runner.hpp"
#include "../../communication_abstractions/rclcpp_callback_communicator.hpp"

namespace performance_test
{
namespace RclcppSteDataRunnerFactory
{
std::shared_ptr<DataRunnerBase> get(const std::string & msg_name, const RunType run_type)
{
  std::shared_ptr<DataRunnerBase> ptr;
  performance_test::for_each(
    messages::MessageTypeList(),
    [&ptr, msg_name, run_type](const auto & msg_type) {
      using T = std::remove_cv_t<std::remove_reference_t<decltype(msg_type)>>;
      if (T::msg_name() == msg_name) {
        ptr = std::make_shared<DataRunner<
          RclcppSingleThreadedExecutorCommunicator<T>>>(run_type);
      }
    });
  if (!ptr) {
    throw std::runtime_error(
            "A topic with the requested name does not exist or communication mean not supported.");
  }
  return ptr;
}
}  // namespace RclcppSteDataRunnerFactory
}  // namespace performance_test
