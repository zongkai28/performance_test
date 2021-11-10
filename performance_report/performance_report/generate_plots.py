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
import yaml
import argparse

from .logs import getTraceConfigs
from .figures import msg_size_vs_latency_cpu, time_vs_latency_cpu_mem
from .utils import create_dir, PerfArgParser


def generate_plots(plot_cfg_file, output_dir):
    with open(plot_cfg_file, "r") as f:
        plots_yaml = yaml.load(f, Loader=yaml.FullLoader)
        trace_configs = getTraceConfigs(plots_yaml["traces"], output_dir)
        try:
            for plot_id, plot_details in plots_yaml["plots"][
                "msg_size_vs_latency_cpu"
            ].items():
                traces_configs = [
                    trace_configs[trace_id] for trace_id in plot_details["traces"]
                ]
                latency_fig, cpu_fig = msg_size_vs_latency_cpu(traces_configs)
                latency_fig.write_image(
                    os.path.join(output_dir, plot_id + "_latency.png")
                )
                cpu_fig.write_image(os.path.join(output_dir, plot_id + "_cpu.png"))
        except KeyError:
            print("No msg_size_vs_latency_cpu plots specified in given trace yaml file")

        try:
            for plot_id, plot_details in plots_yaml["plots"][
                "time_vs_latency_cpu_mem"
            ].items():
                traces_configs = [
                    trace_configs[trace_id] for trace_id in plot_details["traces"]
                ]
                try:
                    latency_fig, cpu_fig, mem_fig = time_vs_latency_cpu_mem(traces_configs)
                except ValueError as e:
                    print(e)
                    print("Skipping generating figure for " + plot_id)
                    continue
                latency_fig.write_image(
                    os.path.join(output_dir, plot_id + "_latency.png")
                )
                cpu_fig.write_image(os.path.join(output_dir, plot_id + "_cpu.png"))
                mem_fig.write_image(os.path.join(output_dir, plot_id + "_mem.png"))
        except KeyError:
            print("No time_vs_latency_cpu_mem plots specified in given trace yaml file")


def main():
    parser = PerfArgParser()
    parser.init_args()
    args = parser.parse_args()
    log_dir = getattr(args, "log_dir")
    test_name = getattr(args, "test_name")
    plot_cfg_files = getattr(args, "configs")

    log_dir = os.path.join(log_dir, test_name)
    for plot_cfg_file in plot_cfg_files:
        generate_plots(plot_cfg_file, log_dir)


if __name__ == "__main__":
    main()
