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

#include "event_db.hpp"

#include <sqlite3.h>

#include <string>
#include <chrono>
#include <stdexcept>
#include <iostream>

namespace
{
const auto SQL_INIT_DB_SCHEMA =
  "DROP TABLE IF EXISTS system_metrics;"
  "CREATE TABLE system_metrics("
  "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
  "  cpu_usage FLOAT,"
  "  ru_utime INT,"
  "  ru_stime INT,"
  "  ru_maxrss INT,"
  "  ru_ixrss INT,"
  "  ru_idrss INT,"
  "  ru_isrss INT,"
  "  ru_minflt INT,"
  "  ru_majflt INT,"
  "  ru_nswap INT,"
  "  ru_inblock INT,"
  "  ru_oublock INT,"
  "  ru_msgsnd INT,"
  "  ru_msgrcv INT,"
  "  ru_nsignals INT,"
  "  ru_nvcsw INT,"
  "  ru_nivcsw INT,"
  "  timestamp INT"
  ");"
  "DROP TABLE IF EXISTS publishers;"
  "CREATE TABLE publishers("
  "  id TEXT PRIMARY KEY,"
  "  msg_type TEXT,"
  "  topic TEXT"
  ");"
  "DROP TABLE IF EXISTS subscribers;"
  "CREATE TABLE subscribers("
  "  id TEXT PRIMARY KEY,"
  "  msg_type TEXT,"
  "  topic TEXT"
  ");"
  "DROP TABLE IF EXISTS messages_sent;"
  "CREATE TABLE messages_sent("
  "  publisher_id TEXT,"
  "  sequence_id INT,"
  "  timestamp INT,"
  "  PRIMARY KEY(publisher_id, sequence_id)"
  ");"
  "DROP TABLE IF EXISTS messages_received;"
  "CREATE TABLE messages_received("
  "  subscriber_id TEXT,"
  "  sequence_id INT,"
  "  timestamp INT,"
  "  PRIMARY KEY(subscriber_id, sequence_id)"
  ");";

const auto SQL_INSERT_PUBLISHERS =
  "INSERT INTO publishers (id, msg_type, topic) VALUES (?, ?, ?);";

const auto SQL_INSERT_SUBSCRIBERS =
  "INSERT INTO subscribers (id, msg_type, topic) VALUES (?, ?, ?);";

const auto SQL_INSERT_MESSAGES_SENT =
  "INSERT INTO messages_sent (publisher_id, sequence_id, timestamp) VALUES (?, ?, ?);";

const auto SQL_INSERT_MESSAGES_RECEIVED =
  "INSERT INTO messages_received (subscriber_id, sequence_id, timestamp) VALUES (?, ?, ?);";

const auto SQL_INSERT_SYSTEM_MEASURED =
  "INSERT INTO messages_received ("
  "  cpu_usage,"
  "  ru_utime,"
  "  ru_stime,"
  "  ru_maxrss,"
  "  ru_ixrss,"
  "  ru_idrss,"
  "  ru_isrss,"
  "  ru_minflt,"
  "  ru_majflt,"
  "  ru_nswap,"
  "  ru_inblock,"
  "  ru_oublock,"
  "  ru_msgsnd,"
  "  ru_msgrcv,"
  "  ru_nsignals,"
  "  ru_nvcsw,"
  "  ru_nivcsw,"
  "  timestamp"
  ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
}  // namespace

namespace performance_test
{

EventDB::EventDB(const std::string & db_file)
{
  int rc = sqlite3_open(db_file.c_str(), &m_db);
  if (rc != SQLITE_OK) {
    throw std::runtime_error("Cannot open database: " + db_file);
  }

  execute(SQL_INIT_DB_SCHEMA);
  sqlite3_prepare_v2(m_db, SQL_INSERT_PUBLISHERS, -1, &m_stmt_register_pub, nullptr);
  sqlite3_prepare_v2(m_db, SQL_INSERT_SUBSCRIBERS, -1, &m_stmt_register_sub, nullptr);
  sqlite3_prepare_v2(m_db, SQL_INSERT_MESSAGES_SENT, -1, &m_stmt_message_sent, nullptr);
  sqlite3_prepare_v2(m_db, SQL_INSERT_MESSAGES_RECEIVED, -1, &m_stmt_message_received, nullptr);
  sqlite3_prepare_v2(m_db, SQL_INSERT_SYSTEM_MEASURED, -1, &m_stmt_system_measured, nullptr);
}

EventDB::~EventDB()
{
  sqlite3_finalize(m_stmt_register_pub);
  sqlite3_finalize(m_stmt_register_sub);
  sqlite3_finalize(m_stmt_message_sent);
  sqlite3_finalize(m_stmt_message_received);
  sqlite3_finalize(m_stmt_system_measured);
  sqlite3_close(m_db);
}

void EventDB::begin_transaction()
{
  execute("BEGIN TRANSACTION");
}

void EventDB::end_transaction()
{
  execute("END TRANSACTION");
}

void EventDB::register_pub(
  const std::string & pub_id, const std::string & msg_type, const std::string & topic)
{
  sqlite3_reset(m_stmt_register_pub);
  sqlite3_bind_text(m_stmt_register_pub, 1, pub_id.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(m_stmt_register_pub, 2, msg_type.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(m_stmt_register_pub, 3, topic.c_str(), -1, SQLITE_STATIC);
  sqlite3_step(m_stmt_register_pub);
}

void EventDB::register_sub(
  const std::string & sub_id, const std::string & msg_type, const std::string & topic)
{
  sqlite3_reset(m_stmt_register_sub);
  sqlite3_bind_text(m_stmt_register_sub, 1, sub_id.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(m_stmt_register_sub, 2, msg_type.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(m_stmt_register_sub, 3, topic.c_str(), -1, SQLITE_STATIC);
  sqlite3_step(m_stmt_register_sub);
}

void EventDB::message_sent(
  const std::string & pub_id, std::uint64_t sequence_id, std::int64_t timestamp)
{
  sqlite3_reset(m_stmt_message_sent);
  sqlite3_bind_text(m_stmt_message_sent, 1, pub_id.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int64(m_stmt_message_sent, 2, static_cast<std::int64_t>(sequence_id));
  sqlite3_bind_int64(m_stmt_message_sent, 3, timestamp);
  sqlite3_step(m_stmt_message_sent);
}

void EventDB::message_received(
  const std::string & sub_id, std::uint64_t sequence_id, std::int64_t timestamp)
{
  sqlite3_reset(m_stmt_message_received);
  sqlite3_bind_text(m_stmt_message_received, 1, sub_id.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int64(m_stmt_message_received, 2, static_cast<std::int64_t>(sequence_id));
  sqlite3_bind_int64(m_stmt_message_received, 3, timestamp);
  sqlite3_step(m_stmt_message_received);
}

namespace
{
int sqlite3_bind_timeval(sqlite3_stmt * stmt, int idx, const timeval & val)
{
  std::chrono::nanoseconds ns =
    std::chrono::seconds(val.tv_sec) +
    std::chrono::microseconds(val.tv_usec);
  return sqlite3_bind_int64(stmt, idx, ns.count());
}
}  // namespace

void EventDB::system_measured(
  const CpuInfo & cpu_info, const rusage & sys_usage, std::int64_t timestamp)
{
  sqlite3_reset(m_stmt_system_measured);
  sqlite3_bind_double(m_stmt_system_measured, 1, cpu_info.cpu_usage());
  sqlite3_bind_timeval(m_stmt_system_measured, 2, sys_usage.ru_utime);
  sqlite3_bind_timeval(m_stmt_system_measured, 3, sys_usage.ru_stime);
  sqlite3_bind_int64(m_stmt_system_measured, 4, sys_usage.ru_maxrss);
  sqlite3_bind_int64(m_stmt_system_measured, 5, sys_usage.ru_ixrss);
  sqlite3_bind_int64(m_stmt_system_measured, 6, sys_usage.ru_idrss);
  sqlite3_bind_int64(m_stmt_system_measured, 7, sys_usage.ru_isrss);
  sqlite3_bind_int64(m_stmt_system_measured, 8, sys_usage.ru_minflt);
  sqlite3_bind_int64(m_stmt_system_measured, 9, sys_usage.ru_majflt);
  sqlite3_bind_int64(m_stmt_system_measured, 10, sys_usage.ru_nswap);
  sqlite3_bind_int64(m_stmt_system_measured, 11, sys_usage.ru_inblock);
  sqlite3_bind_int64(m_stmt_system_measured, 12, sys_usage.ru_oublock);
  sqlite3_bind_int64(m_stmt_system_measured, 13, sys_usage.ru_msgsnd);
  sqlite3_bind_int64(m_stmt_system_measured, 14, sys_usage.ru_msgrcv);
  sqlite3_bind_int64(m_stmt_system_measured, 15, sys_usage.ru_nsignals);
  sqlite3_bind_int64(m_stmt_system_measured, 16, sys_usage.ru_nvcsw);
  sqlite3_bind_int64(m_stmt_system_measured, 17, sys_usage.ru_nivcsw);
  sqlite3_bind_int64(m_stmt_system_measured, 18, timestamp);
  sqlite3_step(m_stmt_system_measured);
}

void EventDB::execute(const std::string & statement)
{
  char * err_msg = 0;
  int rc = sqlite3_exec(m_db, statement.c_str(), nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error: " << err_msg << std::endl;
    sqlite3_free(err_msg);
  }
}

}  // namespace performance_test
