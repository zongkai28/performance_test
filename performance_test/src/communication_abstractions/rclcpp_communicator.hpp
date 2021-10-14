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

#ifndef COMMUNICATION_ABSTRACTIONS__RCLCPP_COMMUNICATOR_HPP_
#define COMMUNICATION_ABSTRACTIONS__RCLCPP_COMMUNICATOR_HPP_


#include <rclcpp/rclcpp.hpp>

#include <memory>
#include <atomic>
#include <utility>

#include "../experiment_configuration/topics.hpp"
#include "../experiment_configuration/qos_abstraction.hpp"

#include "communicator.hpp"
#include "resource_manager.hpp"

namespace performance_test
{

/// Translates abstract QOS settings to specific QOS settings for ROS 2.
class ROS2QOSAdapter
{
public:
  /**
   * \brief The constructor which will save the provided abstract QOS.
   * \param qos The abstract QOS to derive the settings from.
   */
  explicit ROS2QOSAdapter(const QOSAbstraction qos)
  : m_qos(qos) {}
  /// Gets a ROS 2 QOS profile derived from the stored abstract QOS.
  inline rclcpp::QoS get() const
  {
    rclcpp::QoS ros_qos(m_qos.history_depth);

    if (m_qos.reliability == QOSAbstraction::Reliability::BEST_EFFORT) {
      ros_qos.best_effort();
    } else if (m_qos.reliability == QOSAbstraction::Reliability::RELIABLE) {
      ros_qos.reliable();
    } else {
      throw std::runtime_error("Unsupported QOS!");
    }

    if (m_qos.durability == QOSAbstraction::Durability::VOLATILE) {
      ros_qos.durability_volatile();
    } else if (m_qos.durability == QOSAbstraction::Durability::TRANSIENT_LOCAL) {
      ros_qos.transient_local();
    } else {
      throw std::runtime_error("Unsupported QOS!");
    }

    if (m_qos.history_kind == QOSAbstraction::HistoryKind::KEEP_ALL) {
      ros_qos.keep_all();
    } else if (m_qos.history_kind == QOSAbstraction::HistoryKind::KEEP_LAST) {
      ros_qos.keep_last(m_qos.history_depth);
    } else {
      throw std::runtime_error("Unsupported QOS!");
    }

    return ros_qos;
  }

private:
  const QOSAbstraction m_qos;
};

/// Communication plugin interface for ROS 2 for the subscription side.
template<class Msg>
class RclcppCommunicator : public Communicator
{
public:
  /// The data type to publish and subscribe to.
  using DataType = typename Msg::RosType;

  /// Constructor which takes a reference \param lock to the lock to use.
  explicit RclcppCommunicator(EventLogger & event_logger)
  : Communicator(event_logger),
    m_node(ResourceManager::get().rclcpp_node()),
    m_ROS2QOSAdapter(ROS2QOSAdapter(m_ec.qos()).get()),
    m_data_copy(std::make_unique<DataType>()) {}

  /**
   * \brief Publishes a message of type DataType.
   *
   *  The first time this function is called it also creates the publisher.
   *  Further it updates all internal counters while running.
   */
  void publish()
  {
    if (!m_publisher) {
      auto ros2QOSAdapter = m_ROS2QOSAdapter;
      m_publisher = m_node->create_publisher<DataType>(
        m_ec.topic_name() + m_ec.pub_topic_postfix(), ros2QOSAdapter);
#ifdef PERFORMANCE_TEST_APEX_OS_POLLING_SUBSCRIPTION_ENABLED
      if (m_ec.expected_num_subs() > 0) {
        m_publisher->wait_for_matched(
          m_ec.expected_num_subs(),
          m_ec.expected_wait_for_matched_timeout());
      }
#endif
      m_event_logger.register_pub(m_pub_id, m_ec.msg_name(), m_ec.topic_name());
    }
    if (m_ec.is_zero_copy_transfer()) {
      if (!m_publisher->can_loan_messages()) {
        throw std::runtime_error("RMW implementation does not support zero copy!");
      }
      auto borrowed_message{m_publisher->borrow_loaned_message()};
      borrowed_message.get().publisher_id = m_pub_id;
      uint64_t sequence_id = next_sample_id();
      borrowed_message.get().sequence_id = sequence_id;
      m_event_logger.message_sent(m_pub_id, sequence_id, PerfClock::timestamp());
      m_publisher->publish(std::move(borrowed_message));
    } else {
      DataType data;
      data.publisher_id = m_pub_id;
      uint64_t sequence_id = next_sample_id();
      data.sequence_id = sequence_id;
      m_event_logger.message_sent(m_pub_id, sequence_id, PerfClock::timestamp());
      m_publisher->publish(data);
    }
  }

  /// Reads received data from ROS 2 using callbacks
  virtual void update_subscription() = 0;

protected:
  std::shared_ptr<rclcpp::Node> m_node;
  rclcpp::QoS m_ROS2QOSAdapter;
  /**
   * \brief Callback handler which handles the received data.
   *
   * * Verifies that the data arrived in the right order, chronologically and also consistent with the publishing order.
   * * Counts recieved and lost samples.
   * * Calculates the latency of the samples received and updates the statistics accordingly.
   *
   * \param data The data received.
   */
  void callback(const typename DataType::SharedPtr data)
  {
    callback(*data);
  }

  template<class T>
  void callback(const T & data)
  {
    static_assert(
      std::is_same<DataType,
      typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value,
      "Parameter type passed to callback() does not match");

    if (m_ec.roundtrip_mode() == ExperimentConfiguration::RoundTripMode::RELAY) {
      publish();
    } else {
      auto ts = PerfClock::timestamp();
      m_event_logger.message_received(
        m_sub_id, data.publisher_id, data.sequence_id, ts);
    }
  }

private:
  std::shared_ptr<::rclcpp::Publisher<DataType>> m_publisher;
  std::unique_ptr<DataType> m_data_copy;
};
}  // namespace performance_test
#endif  // COMMUNICATION_ABSTRACTIONS__RCLCPP_COMMUNICATOR_HPP_
