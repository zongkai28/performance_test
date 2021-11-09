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

import os
import pandas as pd
import yaml

from .qos import DURABILITY, HISTORY, RELIABILITY
from .transport import TRANSPORT


class PerfConfig:
    def __init__(
        self,
        com_mean: str = "ApexOSPollingSubscription",
        transport: TRANSPORT = TRANSPORT.INTRA,
        msg: str = "Array1k",
        pubs: int = 1,
        subs: int = 1,
        rate: int = 100,
        reliability: RELIABILITY = RELIABILITY.RELIABLE,
        durability: DURABILITY = DURABILITY.VOLATILE,
        history: HISTORY = HISTORY.KEEP_LAST,
        history_depth: int = 16,
        max_runtime: int = 30,
        ignore_seconds: int = 5,
    ) -> None:
        self.com_mean = str(com_mean)
        self.transport = TRANSPORT.coerce(transport)
        self.msg = str(msg)
        self.pubs = int(pubs)
        self.subs = int(subs)
        self.rate = int(rate)
        self.reliability = RELIABILITY.coerce(reliability)
        self.durability = DURABILITY.coerce(durability)
        self.history = HISTORY.coerce(history)
        self.history_depth = int(history_depth)
        self.max_runtime = max_runtime
        self.ignore_seconds = ignore_seconds

    def __eq__(self, o: object) -> bool:
        same = True
        same = same and self.com_mean == o.com_mean
        same = same and self.transport == o.transport
        same = same and self.msg == o.msg
        same = same and self.pubs == o.pubs
        same = same and self.subs == o.subs
        same = same and self.rate == o.rate
        same = same and self.reliability == o.reliability
        same = same and self.durability == o.durability
        same = same and self.history == o.history
        same = same and self.history_depth == o.history_depth
        same = same and self.max_runtime == o.max_runtime
        same = same and self.ignore_seconds == o.ignore_seconds
        return same

    def log_file_name(self) -> str:
        params = [
            self.com_mean,
            self.transport,
            self.msg,
            self.pubs,
            self.subs,
            self.rate,
            self.reliability,
            self.durability,
            self.history,
            self.history_depth,
        ]
        str_params = map(str, params)
        return "_".join(str_params) + ".json"


class FileContents:
    def __init__(self, header: dict, dataframe: pd.DataFrame) -> None:
        self.header = header
        self.dataframe = dataframe


class TraceConfig:
    def __init__(
        self,
        name: str,
        color: str,
        dash: str,
        headers: "list[dict]",
        dataframes: pd.DataFrame,
    ) -> None:
        self.name = name
        self.color = color
        self.dash = dash
        self.headers = headers
        self.dataframes = dataframes


def create_dir(dir_path) -> bool:
    try:
        os.makedirs(dir_path, exist_ok=True)
        return True
    except FileNotFoundError:
        # given path is not viable
        return False


def generate_shmem_file(dir_path) -> str:
    shmem_config_file = os.path.join(dir_path, "shmem.yml")
    if not os.path.exists(shmem_config_file):
        with open(shmem_config_file, "w") as outfile:
            shmem_dict = dict(domain=dict(shared_memory=dict(enable=True)))
            yaml.safe_dump(shmem_dict, outfile)
    return shmem_config_file
