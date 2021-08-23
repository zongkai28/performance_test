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

#include "qos_abstraction.hpp"

#include <string>

namespace performance_test
{

std::string to_string(const QOSAbstraction::Reliability e)
{
  if (e == QOSAbstraction::Reliability::BEST_EFFORT) {
    return "BEST_EFFORT";
  } else if (e == QOSAbstraction::Reliability::RELIABLE) {
    return "RELIABLE";
  } else {
    throw std::invalid_argument("Enum value not supported!");
  }
}

std::string to_string(const QOSAbstraction::Durability e)
{
  if (e == QOSAbstraction::Durability::VOLATILE) {
    return "VOLATILE";
  } else if (e == QOSAbstraction::Durability::TRANSIENT_LOCAL) {
    return "TRANSIENT_LOCAL";
  } else {
    throw std::invalid_argument("Enum value not supported!");
  }
}

std::string to_string(const QOSAbstraction::HistoryKind e)
{
  if (e == QOSAbstraction::HistoryKind::KEEP_ALL) {
    return "KEEP_ALL";
  } else if (e == QOSAbstraction::HistoryKind::KEEP_LAST) {
    return "KEEP_LAST";
  } else {
    throw std::invalid_argument("Enum value not supported!");
  }
}

std::ostream & operator<<(std::ostream & stream, const QOSAbstraction::Reliability e)
{
  return stream << to_string(e);
}

std::ostream & operator<<(std::ostream & stream, const QOSAbstraction::Durability e)
{
  return stream << to_string(e);
}

std::ostream & operator<<(std::ostream & stream, const QOSAbstraction::HistoryKind e)
{
  return stream << to_string(e);
}

std::ostream & operator<<(std::ostream & stream, const QOSAbstraction e)
{
  return stream <<
         "Reliability: " << e.reliability <<
         " Durability: " << e.durability <<
         " History kind: " << e.history_kind <<
         " History depth: " << e.history_depth <<
         " Sync. pub/sub: " << e.sync_pubsub;
}

}  // namespace performance_test
