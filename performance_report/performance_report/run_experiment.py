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
import time
import yaml

from .logs import getExperiments, getExperimentLogPath
from .utils import cliColors, create_dir, generate_shmem_file, ExperimentConfig, PerfArgParser
from .qos import DURABILITY, HISTORY, RELIABILITY
from .transport import TRANSPORT


def prepare_for_shmem(cfg: ExperimentConfig, output_dir):
    # TODO(flynneva): check cfg.com_mean if these are applicable
    if cfg.transport == TRANSPORT.ZERO_COPY or cfg.transport == TRANSPORT.SHMEM:
        shmem_config_file = generate_shmem_file(output_dir)
        print(cliColors.WARN + "[Warning] RouDi is expected to already be running" + cliColors.ENDCOLOR)
        os.environ["APEX_MIDDLEWARE_SETTINGS"] = shmem_config_file
        os.environ["CYCLONEDDS_URI"] = shmem_config_file


def teardown_from_shmem(cfg: ExperimentConfig):
    if cfg.transport == TRANSPORT.ZERO_COPY or cfg.transport == TRANSPORT.SHMEM:
        os.unsetenv("APEX_MIDDLEWARE_SETTINGS")
        os.unsetenv("CYCLONEDDS_URI")


def run_experiment(cfg: ExperimentConfig, output_dir, overwrite: bool):
    lf = getExperimentLogPath(output_dir, cfg)
    if os.path.exists(lf) and not overwrite:
        print(cliColors.WARN + f"Skipping experiment {cfg.log_file_name()} as results already exist in " + output_dir + cliColors.ENDCOLOR)
        return
    else:
        print(cliColors.GREEN + f"Running experiment {cfg.log_file_name()}" + cliColors.ENDCOLOR)

    cmd = "ros2 run performance_test perf_test"
    cmd += f" -c {cfg.com_mean}"
    cmd += f" -m {cfg.msg}"
    cmd += f" -r {cfg.rate}"
    if cfg.reliability == RELIABILITY.RELIABLE:
        cmd += " --reliable"
    if cfg.durability == DURABILITY.TRANSIENT_LOCAL:
        cmd += " --transient"
    if cfg.history == HISTORY.KEEP_LAST:
        cmd += " --keep-last"
    cmd += f" --history-depth {cfg.history_depth}"
    cmd += f" --use-rt-prio {cfg.rt_prio}"
    cmd += f" --use-rt-cpus {cfg.rt_cpus}"
    cmd += f" --max-runtime {cfg.max_runtime}"
    cmd += f" --ignore {cfg.ignore_seconds}"
    if cfg.transport == TRANSPORT.INTRA:
        cmd += f" -p {cfg.pubs} -s {cfg.subs} -o json --json-logfile {lf}"
        os.system(cmd)
    else:
        cmd_sub = cmd + f" -p 0 -s {cfg.subs} --expected-num-pubs {cfg.pubs} -o json --json-logfile {lf}"
        cmd_pub = cmd + f" -s 0 -p {cfg.pubs} --expected-num-subs {cfg.subs} -o none"
        if cfg.transport == TRANSPORT.ZERO_COPY:
            cmd_sub += " --zero-copy"
            cmd_pub += " --zero-copy"
        prepare_for_shmem(cfg, output_dir)
        os.system(cmd_sub + " &")
        time.sleep(1)
        os.system(cmd_pub)
        teardown_from_shmem(cfg)


def run_experiments(files: "list[str]", output_dir, overwrite: bool):
    # make sure output dir exists
    create_dir(output_dir)
    # loop over given run files and run experiments
    for run_file in files:
        with open(run_file, "r") as f:
            run_cfg = yaml.load(f, Loader=yaml.FullLoader)

        run_configs = getExperiments(run_cfg["experiments"])

        for run_config in run_configs:
            run_experiment(run_config, output_dir, overwrite)


def main():
    parser = PerfArgParser()
    parser.init_args()
    args = parser.parse_args()
    
    log_dir = getattr(args, "log_dir")
    test_name = getattr(args, "test_name")
    run_files = getattr(args, "configs")
    overwrite = bool(getattr(args, "force"))

    log_dir = os.path.join(log_dir, test_name)
    run_experiments(run_files, log_dir, overwrite)


# if this file is called directly
if __name__ == "__main__":
    main()
