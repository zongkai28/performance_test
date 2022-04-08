// Copyright 2019 ADLINK Techonology, Inc.
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

#ifndef COMMUNICATION_ABSTRACTIONS__CYCLONEDDS_COMMUNICATOR_HPP_
#define COMMUNICATION_ABSTRACTIONS__CYCLONEDDS_COMMUNICATOR_HPP_

#include <dds/dds.h>
#include <dds/ddsc/dds_loan_api.h>

#include <string>

#include "communicator.hpp"
#include "resource_manager.hpp"

namespace performance_test
{

/**
 * \brief Translates abstract QOS settings to specific QOS settings for Connext DDS
 * Micro data writers and readers.
 *
 * The reason that this class is constructed like this is that one usually gets a
 * partially specified QOS from the topic or similar higher level entity and just
 * changes some settings from these.
 */
class CycloneDDSQOSAdapter
{
public:
  /**
   * \brief Constructs the QOS adapter.
   * \param qos The abstract QOS settings the adapter should use to derive the
   * implementation specific QOS settings.
   */
  explicit CycloneDDSQOSAdapter(const QOSAbstraction qos)
  : m_qos(qos)
  {}
  /**
   * \brief Applies the abstract QOS to an existing QOS leaving unsupported values as
   * they were.
   * \tparam CycloneDDSQos The type of the QOS setting, for example data reader or data
   * writer QOS.
   * \param qos The QOS settings to fill supported values in.
   */
  template<class CycloneDDSQos>
  void apply(CycloneDDSQos & qos)
  {
    if (m_qos.reliability == QOSAbstraction::Reliability::BEST_EFFORT) {
      dds_qset_reliability(qos, DDS_RELIABILITY_BEST_EFFORT, DDS_SECS(1));
    } else if (m_qos.reliability == QOSAbstraction::Reliability::RELIABLE) {
      dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(1));
    } else {
      throw std::runtime_error("Unsupported QOS!");
    }

    if (m_qos.durability == QOSAbstraction::Durability::VOLATILE) {
      dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);
    } else if (m_qos.durability == QOSAbstraction::Durability::TRANSIENT_LOCAL) {
      dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
    } else {
      throw std::runtime_error("Unsupported QOS!");
    }

    if (m_qos.history_kind == QOSAbstraction::HistoryKind::KEEP_ALL) {
      dds_qset_history(qos, DDS_HISTORY_KEEP_ALL, 1);
    } else if (m_qos.history_kind == QOSAbstraction::HistoryKind::KEEP_LAST) {
      dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, static_cast<int32_t>(m_qos.history_depth));
    } else {
      throw std::runtime_error("Unsupported QOS!");
    }
  }

private:
  const QOSAbstraction m_qos;
};

/**
 * \brief The plugin for Cyclone DDS.
 * \tparam Msg The msg type to use.
 */
template<class Msg>
class CycloneDDSCommunicator : public Communicator
{
public:
  /// The data type to use.
  using DataType = typename Msg::CycloneDDSType;

  /// Constructor which takes a reference \param lock to the lock to use.
  explicit CycloneDDSCommunicator(SpinLock & lock)
  : Communicator(lock),
    m_participant(ResourceManager::get().cyclonedds_participant()),
    m_datawriter(0),
    m_datareader(0)
  {
  }

  /**
   * \brief Publishes the provided data.
   *
   *  The first time this function is called it also creates the data writer.
   *  Further it updates all internal counters while running.
   * \param data The data to publish.
   * \param time The time to fill into the data field.
   */
  void publish(std::int64_t time)
  {
    if (m_datawriter == 0) {
      dds_qos_t * dw_qos = dds_create_qos();
      CycloneDDSQOSAdapter qos_adapter(m_ec.qos());
      qos_adapter.apply(dw_qos);
      dds_entity_t tp = create_topic(m_ec.pub_topic_postfix());
      m_datawriter = dds_create_writer(m_participant, tp, dw_qos, nullptr);
      dds_delete(tp);
      dds_delete_qos(dw_qos);
      if (m_datawriter < 0) {
        throw std::runtime_error("failed to create datawriter");
      }
    }
    lock();
    init_msg(m_data, time);
    increment_sent();  // We increment before publishing so we don't have to lock twice.
    unlock();
    if (dds_write(m_datawriter, static_cast<void *>(&m_data)) < 0) {
      throw std::runtime_error("Failed to write to sample");
    }
  }

  /**
   * \brief Reads received data from DDS.
   *
   * In detail this function:
   * * Reads samples from DDS.
   * * Verifies that the data arrived in the right order, chronologically and also
   *   consistent with the publishing order.
   * * Counts received and lost samples.
   * * Calculates the latency of the samples received and updates the statistics
       accordingly.
   */
  void update_subscription()
  {
    if (m_datareader == 0) {
      dds_qos_t * dw_qos = dds_create_qos();
      CycloneDDSQOSAdapter qos_adapter(m_ec.qos());
      qos_adapter.apply(dw_qos);
      dds_entity_t tp = create_topic(m_ec.sub_topic_postfix());
      m_datareader = dds_create_reader(m_participant, tp, dw_qos, nullptr);
      dds_delete(tp);
      dds_delete_qos(dw_qos);
      if (m_datareader < 0) {
        throw std::runtime_error("failed to create datareader");
      }
      dds_set_status_mask(m_datareader, DDS_DATA_AVAILABLE_STATUS);
      m_waitset = dds_create_waitset(m_participant);
      if (dds_waitset_attach(m_waitset, m_datareader, 1) < 0) {
        throw std::runtime_error("failed to attach waitset");
      }
    }

    dds_waitset_wait(m_waitset, nullptr, 0, DDS_SECS(15));

    void * untyped = nullptr;
    dds_sample_info_t si;
    int32_t n;
    while ((n = dds_take(m_datareader, &untyped, &si, 1, 1)) > 0) {
      lock();
      const DataType * data = static_cast<DataType *>(untyped);
      if (si.valid_data) {
        if (m_prev_timestamp >= data->time) {
          throw std::runtime_error(
                  "Data consistency violated. Received sample with not strictly older timestamp. "
                  "Time diff: " + std::to_string(data->time - m_prev_timestamp) +
                  " Data Time: " + std::to_string(data->time));
        }
        if (m_ec.roundtrip_mode() == ExperimentConfiguration::RoundTripMode::RELAY) {
          unlock();
          publish(data->time);
          lock();
        } else {
          m_prev_timestamp = data->time;
          update_lost_samples_counter(data->id);
          add_latency_to_statistics(data->time);
          increment_received();
        }
      }
      unlock();

      dds_return_loan(m_datareader, &untyped, n);
    }
  }

  /// Returns the data received in bytes.
  std::size_t data_received()
  {
    return num_received_samples() * sizeof(DataType);
  }

private:
  /// Creates a new topic for the participant
  dds_entity_t create_topic(const std::string & postfix)
  {
    dds_entity_t topic;
    topic = dds_create_topic(
      m_participant, Msg::CycloneDDSDesc(),
      (m_ec.topic_name() + postfix).c_str(), nullptr, nullptr);
    if (topic < 0) {
      throw std::runtime_error("failed to create topic");
    }
    return topic;
  }

  dds_entity_t m_participant;

  dds_entity_t m_datawriter;
  dds_entity_t m_datareader;

  dds_entity_t m_waitset;
  dds_entity_t m_condition;

  DataType m_data;
};

}  // namespace performance_test

#endif  // COMMUNICATION_ABSTRACTIONS__CYCLONEDDS_COMMUNICATOR_HPP_
