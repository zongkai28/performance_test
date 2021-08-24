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

#ifndef UTILITIES__CPU_INFO_HPP_
#define UTILITIES__CPU_INFO_HPP_

#include <cstdint>

namespace performance_test
{
class CpuInfo
{
public:
  CpuInfo(uint32_t cpu_cores, float cpu_usage)
  : m_cpu_cores(cpu_cores), m_cpu_usage(cpu_usage) {}

  uint32_t cpu_cores() const
  {
    return m_cpu_cores;
  }

  float cpu_usage() const
  {
    return m_cpu_usage;
  }

private:
  uint32_t m_cpu_cores;
  float m_cpu_usage;
};
}  // namespace performance_test

#endif  // UTILITIES__CPU_INFO_HPP_
