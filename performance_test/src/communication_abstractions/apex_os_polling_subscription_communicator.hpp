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

#ifndef COMMUNICATION_ABSTRACTIONS__APEX_OS_POLLING_SUBSCRIPTION_COMMUNICATOR_HPP_
#define COMMUNICATION_ABSTRACTIONS__APEX_OS_POLLING_SUBSCRIPTION_COMMUNICATOR_HPP_

#include <rclcpp/rclcpp.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "../experiment_configuration/topics.hpp"
#include "../experiment_configuration/qos_abstraction.hpp"

#include "rclcpp_communicator.hpp"
#include "resource_manager.hpp"

namespace performance_test
{

/// Communication plugin for Apex.OS using waitsets for the subscription side.
template<class Msg>
class ApexOSPollingSubscriptionCommunicator : public RclcppCommunicator<Msg>
{
public:
  /// The data type to publish and subscribe to.
  using DataType = typename RclcppCommunicator<Msg>::DataType;

  /// Constructor which takes a reference \param lock to the lock to use.
  explicit ApexOSPollingSubscriptionCommunicator(EventLogger & event_logger)
  : RclcppCommunicator<Msg>(event_logger),
    m_polling_subscription(nullptr) {}

  /// Reads received data from ROS 2 using waitsets
  void update_subscription() override
  {
    if (!m_polling_subscription) {
      m_polling_subscription = this->m_node->template create_polling_subscription<DataType>(
        this->m_ec.topic_name() + this->m_ec.sub_topic_postfix(), this->m_ROS2QOSAdapter);
      if (this->m_ec.expected_num_pubs() > 0) {
        m_polling_subscription->wait_for_matched(
          this->m_ec.expected_num_pubs(),
          this->m_ec.expected_wait_for_matched_timeout());
      }
      m_waitset = std::make_unique<rclcpp::Waitset<>>(m_polling_subscription);
      this->m_event_logger.register_sub(
        this->m_sub_id, this->m_ec.msg_name(), this->m_ec.topic_name(), sizeof(DataType));
    }
    const auto wait_ret = m_waitset->wait(std::chrono::milliseconds(100), false);
    if (wait_ret.any()) {
      const auto loaned_msg = m_polling_subscription->take(RCLCPP_LENGTH_UNLIMITED);
      for (const auto msg : loaned_msg) {
        if (msg.info().valid()) {
          this->template callback(msg.data());
        }
      }
    }
  }

private:
  using PollingSubscriptionType = ::rclcpp::PollingSubscription<DataType>;
  std::shared_ptr<PollingSubscriptionType> m_polling_subscription;
  std::unique_ptr<rclcpp::Waitset<>> m_waitset;
};

}  // namespace performance_test

#endif  // COMMUNICATION_ABSTRACTIONS__APEX_OS_POLLING_SUBSCRIPTION_COMMUNICATOR_HPP_
