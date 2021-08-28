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

#ifndef COMMUNICATION_ABSTRACTIONS__ICEORYX_COMMUNICATOR_HPP_
#define COMMUNICATION_ABSTRACTIONS__ICEORYX_COMMUNICATOR_HPP_

#include <iceoryx_posh/popo/publisher.hpp>
#include <iceoryx_posh/popo/subscriber.hpp>

#include <string>
#include <memory>

#include "communicator.hpp"
#include "resource_manager.hpp"

namespace performance_test
{
/**
 * \brief The plugin for iceoryx.
 * \tparam Msg The msg type to use.
 */
template<class Msg>
class IceoryxCommunicator : public Communicator
{
public:
  /// The data type to use.
  using DataType = typename Msg::RosType;

  /// Constructor which takes a reference \param lock to the lock to use.
  explicit IceoryxCommunicator(EventLogger & event_logger)
  : Communicator(event_logger)
  {
  }

  /**
   * \brief Publishes a message of type DataType.
   *
   *  The first time this function is called it also creates the publisher.
   *  Further it updates all internal counters while running.
   */
  void publish()
  {
    if (m_publisher == nullptr) {
      ResourceManager::get().init_iceoryx_runtime();
      iox::capro::IdString_t iox_pub_service{iox::cxx::TruncateToCapacity, Msg::msg_name()};
      iox::capro::IdString_t iox_pub_instance{iox::cxx::TruncateToCapacity, m_ec.topic_name()};
      iox::capro::IdString_t iox_pub_event{"Object"};
      m_publisher = std::unique_ptr<iox::popo::Publisher<DataType>>(
        new iox::popo::Publisher<DataType>({iox_pub_service, iox_pub_instance, iox_pub_event}));
    }

    if (m_ec.is_zero_copy_transfer()) {
      m_publisher->loan()
      .and_then(
        [&](auto & sample) {
          uint64_t sequence_id = next_sample_id();
          sample->id = sequence_id;
          m_event_logger.message_sent(m_pub_id, sequence_id, PerfClock::timestamp());
          sample.publish();
        })
      .or_else(
        [](auto &) {
          throw std::runtime_error("Failed to write to sample");
        });
    } else {
      DataType data;
      uint64_t sequence_id = next_sample_id();
      data.id = sequence_id;
      m_event_logger.message_sent(m_pub_id, sequence_id, PerfClock::timestamp());
      m_publisher->publishCopyOf(data)
      .or_else(
        [](auto &) {
          throw std::runtime_error("Failed to write to sample");
        });
    }
  }

  /**
   * \brief Reads received data.
   *
   * The first time this function is called it also creates the subscriber.
   * In detail this function:
   * * Reads samples.
   * * Verifies that the data arrived in the right order, chronologically and also
   *   consistent with the publishing order.
   * * Counts received and lost samples.
   * * Calculates the latency of the samples received and updates the statistics
       accordingly.
   */
  void update_subscription()
  {
    if (m_subscriber == nullptr) {
      ResourceManager::get().init_iceoryx_runtime();
      iox::capro::IdString_t iox_sub_service{iox::cxx::TruncateToCapacity, Msg::msg_name()};
      iox::capro::IdString_t iox_sub_instance{iox::cxx::TruncateToCapacity, m_ec.topic_name()};
      iox::capro::IdString_t iox_sub_event{"Object"};
      iox::popo::SubscriberOptions subscriberOptions;
      subscriberOptions.queueCapacity = m_ec.qos().history_depth;
      m_subscriber = std::unique_ptr<iox::popo::Subscriber<DataType>>(
        new iox::popo::Subscriber<DataType>(
          {iox_sub_service, iox_sub_instance, iox_sub_event},
          subscriberOptions));
      m_waitset = std::unique_ptr<iox::popo::WaitSet<>>(new iox::popo::WaitSet<>());
      m_waitset->attachEvent(*m_subscriber.get(), iox::popo::SubscriberEvent::DATA_RECEIVED)
      .or_else(
        [](
          auto) {
          std::cerr << "unable to attach Event DATA_RECEIVED to iceoryx Waitset" << std::endl;
          std::exit(EXIT_FAILURE);
        });
      m_event_logger.register_sub(m_sub_id, m_ec.msg_name(), m_ec.topic_name(), sizeof(DataType));
    }

    if (m_subscriber->getSubscriptionState() == iox::SubscribeState::SUBSCRIBED) {
      auto eventVector = m_waitset->timedWait(iox::units::Duration::fromSeconds(15));
      for (auto & event : eventVector) {
        if (event->doesOriginateFrom(m_subscriber.get())) {
          while (m_subscriber->hasData()) {
            m_subscriber->take()
            .and_then(
              [this](auto & data) {
                m_event_logger.message_received(
                  m_sub_id, data->id, PerfClock::timestamp());
              })
            .or_else(
              [](auto & result) {
                if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE) {
                  throw std::runtime_error("Error: received Chunk not available");
                }
              });
          }
        }
      }
    } else {
      std::cout << "Not subscribed!" << std::endl;
    }
  }

private:
  std::unique_ptr<iox::popo::Publisher<DataType>> m_publisher;
  std::unique_ptr<iox::popo::Subscriber<DataType>> m_subscriber;
  std::unique_ptr<iox::popo::WaitSet<>> m_waitset;
};

}  // namespace performance_test

#endif  // COMMUNICATION_ABSTRACTIONS__ICEORYX_COMMUNICATOR_HPP_
