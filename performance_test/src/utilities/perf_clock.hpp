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

#ifndef UTILITIES__PERF_CLOCK_HPP_
#define UTILITIES__PERF_CLOCK_HPP_

#include <chrono>

#if defined(QNX)
#include <sys/neutrino.h>
#include <inttypes.h>
#endif

namespace performance_test
{

class PerfClock {
private:
#ifdef QNX710
  using perf_clock = std::chrono::system_clock;
#else
  using perf_clock = std::chrono::steady_clock;
#endif
public:
  using time_point = perf_clock::time_point;

  static perf_clock::time_point now() {
    return perf_clock::now();
  }

  static std::int64_t timestamp() {
#if defined(QNX)
    return static_cast<std::int64_t>(ClockCycles());
#else
    return perf_clock::now().time_since_epoch().count();
#endif
  }
};
}  // namespace performance_test

#endif  // UTILITIES__PERF_CLOCK_HPP_
