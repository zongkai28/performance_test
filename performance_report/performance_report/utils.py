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
import argparse
import os
import sys
import pandas as pd
import yaml
import re

from .qos import DURABILITY, HISTORY, RELIABILITY
from .transport import TRANSPORT


# Describes a single experiment to be run
class ExperimentConfig:
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
        rt_prio: int = 0,
        rt_cpus: int = 0,
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
        self.rt_prio = int(rt_prio)
        self.rt_cpus = int(rt_cpus)
        self.max_runtime = int(max_runtime)
        self.ignore_seconds = int(ignore_seconds)

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
        same = same and self.rt_prio == o.rt_prio
        same = same and self.rt_cpus == o.rt_cpus
        same = same and self.max_runtime == o.max_runtime
        same = same and self.ignore_seconds == o.ignore_seconds
        return same

    def log_file_name(self) -> str:
        # Keep this in sync with ExperimentFilterConfig
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
            self.rt_prio,
            self.rt_cpus,
        ]
        str_params = map(str, params)
        return "_".join(str_params) + ".json"
    
    def from_log_file(filename):
        params = filename.strip('.json').split('_')
        return ExperimentConfig(*params)

    def as_dataframe(self) -> pd.DataFrame:
        return pd.DataFrame({
            'com_mean': self.com_mean,
            'transport': self.transport,
            'msg': self.msg,
            'pubs': self.pubs,
            'subs': self.subs,
            'rate': self.rate,
            'reliability': self.reliability,
            'durability': self.durability,
            'history': self.history,
            'history_depth': self.history_depth,
            'rt_prio': self.rt_prio,
            'rt_cpus': self.rt_cpus,
            'max_runtime': self.max_runtime,
            'ignore_seconds': self.ignore_seconds,
        }, index=[0])
    
    def get_members(self) -> list:
        members = []
        for attribute in dir(self):
            if not callable(getattr(self, attribute)) and \
               not attribute.startswith('__'):
               members.append(attribute)
        return members

    def cli_commands(self, perf_test_exe_cmd, output_dir) -> list:
        args = self.cli_args(output_dir)
        commands = []
        if len(args) == 1:
            commands.append(perf_test_exe_cmd + args[0])
        elif len(args) == 2:
            sub_args, pub_args = args
            if self.transport == TRANSPORT.ZERO_COPY or self.transport == TRANSPORT.SHMEM:
                shmem_config_file = os.path.join(output_dir, "shmem.yml")
                commands.append(f'export APEX_MIDDLEWARE_SETTINGS="{shmem_config_file}"')
                commands.append(f'export CYCLONEDDS_URI="{shmem_config_file}"')
                commands.append("echo -e 'domain:\n  shared_memory:\n    enable: true' > ${APEX_MIDDLEWARE_SETTINGS}")
            commands.append(sub_args + ' &')
            commands.append('sleep 1')
            commands.append(pub_args)
            if self.transport == TRANSPORT.ZERO_COPY or self.transport == TRANSPORT.SHMEM:
                commands.append('unset APEX_MIDDLEWARE_SETTINGS')
                commands.append('unset CYCLONEDDS_URI')
        else:
            raise RuntimeError('Unreachable code')
        return commands

    def cli_args(self, output_dir) -> list:
        args = ""
        args += f" -c {self.com_mean}"
        args += f" -m {self.msg}"
        args += f" -r {self.rate}"
        if self.reliability == RELIABILITY.RELIABLE:
            args += " --reliable"
        if self.durability == DURABILITY.TRANSIENT_LOCAL:
            args += " --transient"
        if self.history == HISTORY.KEEP_LAST:
            args += " --keep-last"
        args += f" --history-depth {self.history_depth}"
        args += f" --use-rt-prio {self.rt_prio}"
        args += f" --use-rt-cpus {self.rt_cpus}"
        args += f" --max-runtime {self.max_runtime}"
        args += f" --ignore {self.ignore_seconds}"
        lf = os.path.join(output_dir, self.log_file_name())
        if self.transport == TRANSPORT.INTRA:
            args += f" -p {self.pubs} -s {self.subs} -o json --json-logfile {lf}"
            return [args]
        else:
            args_sub = args + f" -p 0 -s {self.subs} --expected-num-pubs {self.pubs} -o json --json-logfile {lf}"
            args_pub = args + f" -s 0 -p {self.pubs} --expected-num-subs {self.subs} -o none"
            if self.transport == TRANSPORT.ZERO_COPY:
                args_sub += " --zero-copy"
                args_pub += " --zero-copy"
            return [args_sub, args_pub]


# Describes how to select one or more completed experiments
class ExperimentFilterConfig:
    def __init__(
        self,
        com_mean: str = '.*',
        transport: str = '.*',
        msg: str = '.*',
        pubs: str = '.*',
        subs: str = '.*',
        rate: str = '.*',
        reliability: str = '.*',
        durability: str = '.*',
        history: str = '.*',
        history_depth: str = '.*',
        rt_prio: str = '.*',
        rt_cpus: str = '.*',
    ) -> None:
        self.com_mean = self.wildcard_or_coerce(com_mean, str)
        self.transport = self.wildcard_or_coerce(transport, TRANSPORT.coerce)
        self.msg = self.wildcard_or_coerce(msg, str)
        self.pubs = self.wildcard_or_coerce(pubs, int)
        self.subs = self.wildcard_or_coerce(subs, int)
        self.rate = self.wildcard_or_coerce(rate, int)
        self.reliability = self.wildcard_or_coerce(reliability, RELIABILITY.coerce)
        self.durability = self.wildcard_or_coerce(durability, DURABILITY.coerce)
        self.history = self.wildcard_or_coerce(history, HISTORY.coerce)
        self.history_depth = self.wildcard_or_coerce(history_depth, int)
        self.rt_prio = self.wildcard_or_coerce(rt_prio, int)
        self.rt_cpus = self.wildcard_or_coerce(rt_cpus, int)

    def wildcard_or_coerce(self, param: str, coerce: 'function'):
        if param == '.*':
            return param
        else:
            return coerce(param)

    def file_regex(self):
        # Keep this in sync with ExperimentConfig
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
            self.rt_prio,
            self.rt_cpus,
        ]
        str_params = map(str, params)
        return re.compile("_".join(str_params) + ".json")


class LineConfig:
    def __init__(
        self,
        style: str = 'solid',
        width: int = 2,
        alpha: float = 1.0
    ) -> None:
        self.style = style
        self.width = width
        self.alpha = alpha


class MarkerConfig:
    def __init__(
        self,
        shape: str = 'dot',
        size: int = 25,
        alpha: float = 1.0
    ) -> None:
       self.shape = str(shape)
       self.size = size
       self.alpha = alpha


class ThemeConfig:
    def __init__(
        self,
        color: str = '#0000ff',
        marker: MarkerConfig = MarkerConfig(),
        line: LineConfig = LineConfig(),
    ) -> None:
        self.color = str(color)
        self.marker = marker
        self.line = line


class DatasetConfig:
    def __init__(
        self,
        name: str = 'default_dataset',
        theme: ThemeConfig = ThemeConfig(),
        experiments: [ExperimentConfig] = [ExperimentConfig()],
        headers: [(str, dict)] = ['default_experiment', {}],
        dataframe: pd.DataFrame = pd.DataFrame(),
    ) -> None:
        self.name = name
        self.theme = theme
        self.experiments = experiments
        self.headers = headers
        self.dataframe = dataframe


class FileContents:
    def __init__(self, header: dict, dataframe: pd.DataFrame) -> None:
        self.header = header
        self.dataframe = dataframe


class PerfArgParser(argparse.ArgumentParser):

    def init_args(self):
        self.add_argument(
            "--log-dir",
            "-l",
            default='.',
            help="The directory for the perf_test log files and plot images",
        )
        self.add_argument(
            "--test-name",
            "-t",
            default="experiment",
            help="Name of the experiment set to help give context to the test results",
        )
        self.add_argument(
            "--configs",
            "-c",
            default=[],
            nargs="+",
            help="The yaml file(s) containing experiments to run",
        )
        self.add_argument(
            "--perf-test-exe",
            default='ros2 run performance_test perf_test',
            help="The command to run the perf_test executable",
        )
        self.add_argument(
            "--force",
            "-f",
            action="store_true",
            help="Force existing results to be overwritten (by default, they are skipped).",
        )

        if len(sys.argv) == 1:
            print('[ERROR][ %s ] No arguments given\n' % self.prog)
            self.print_help()
            sys.exit(2)
        elif (sys.argv[1] == '-h' or sys.argv[1] == '--help'):
            self.print_help()
            sys.exit(0)

    def error(self, msg):
        print('[ERROR][ %s ] %s\n' % (self.prog, msg))
        self.print_help()
        sys.exit(2)

    def exit(self, msg):
        print('EXIT')


class cliColors:
    ESCAPE = '\033'
    GREEN = ESCAPE + '[92m'
    WARN = ESCAPE + '[93m'
    ERROR = ESCAPE + '[91m'
    ENDCOLOR = ESCAPE + '[0m'


def create_dir(dir_path) -> bool:
    try:
        os.makedirs(dir_path)
        return True
    except FileExistsError:
        print(cliColors.WARN + 'Log directory already exists' + cliColors.ENDCOLOR)
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
