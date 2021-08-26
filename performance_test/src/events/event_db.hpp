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

#include "../events/event_sink.hpp"
#include "../utilities/cpu_info.hpp"

namespace performance_test
{
class EventDB : public EventSink
{
public:
  explicit EventDB(const std::string & db_file);
  ~EventDB();
  void begin_transaction();
  void end_transaction();
  void register_pub(const EventRegisterPub & event);
  void register_sub(const EventRegisterSub & event);
  void message_sent(const EventMessageSent & event);
  void message_received(const EventMessageReceived & event);
  void system_measured(const EventSystemMeasured & event);

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
