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

#ifndef EVENTS__EVENT_DB_HPP_
#define EVENTS__EVENT_DB_HPP_

#include <sqlite3.h>
#include <sys/resource.h>

#include <string>

#include "../utilities/cpu_info.hpp"

namespace performance_test
{
class EventDB
{
public:
  explicit EventDB(const std::string & db_file);
  ~EventDB();
  void begin_transaction();
  void end_transaction();
  void register_pub(
    const std::string & pub_id, const std::string & msg_type, const std::string & topic);
  void register_sub(
    const std::string & sub_id, const std::string & msg_type, const std::string & topic);
  void message_sent(
    const std::string & pub_id, std::uint64_t sequence_id, std::int64_t timestamp);
  void message_received(
    const std::string & sub_id, std::uint64_t sequence_id, std::int64_t timestamp);
  void system_measured(
    const CpuInfo & cpu_info, const rusage & sys_usage, std::int64_t timestamp);

private:
  void execute(const std::string & statement);
  sqlite3 * m_db;
  sqlite3_stmt * m_stmt_register_pub;
  sqlite3_stmt * m_stmt_register_sub;
  sqlite3_stmt * m_stmt_message_sent;
  sqlite3_stmt * m_stmt_message_received;
  sqlite3_stmt * m_stmt_system_measured;
};
}  // namespace performance_test

#endif  // EVENTS__EVENT_DB_HPP_
