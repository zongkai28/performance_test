# Copyright 2021 Apex.AI, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import json
from enum import Enum


class DURABILITY(int, Enum):
    VOLATILE = 1
    TRANSIENT_LOCAL = 2

    def __str__(self):
        if self == DURABILITY.VOLATILE:
            return "V"
        if self == DURABILITY.TRANSIENT_LOCAL:
            return "T"

    def coerce(val):
        if isinstance(val, DURABILITY):
            return val
        if isinstance(val, int):
            return DURABILITY(val)
        if isinstance(val, str):
            return DURABILITY[val]
        raise TypeError


class HISTORY(int, Enum):
    KEEP_ALL = 1
    KEEP_LAST = 2

    def __str__(self):
        if self == HISTORY.KEEP_ALL:
            return "A"
        if self == HISTORY.KEEP_LAST:
            return "L"

    def coerce(val):
        if isinstance(val, HISTORY):
            return val
        if isinstance(val, int):
            return HISTORY(val)
        if isinstance(val, str):
            return HISTORY[val]
        raise TypeError


class RELIABILITY(int, Enum):
    RELIABLE = 1
    BEST_EFFORT = 2

    def __str__(self):
        if self == RELIABILITY.RELIABLE:
            return "R"
        if self == RELIABILITY.BEST_EFFORT:
            return "B"

    def coerce(val):
        if isinstance(val, RELIABILITY):
            return val
        if isinstance(val, int):
            return RELIABILITY(val)
        if isinstance(val, str):
            return RELIABILITY[val]
        raise TypeError
