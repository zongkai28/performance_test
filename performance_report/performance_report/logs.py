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

import itertools
import json
import pandas as pd
import os


from .utils import PerfConfig, DatasetConfig


def parseLog(filename):
    """Load logfile into header dictionary and pandas dataframe."""
    header = ''
    dataframe = pd.DataFrame()
    try:
        with open(filename) as source:
            try:
                header = json.load(source)
            except json.decoder.JSONDecodeError:
                print("Unable to decode JSON file " + filename)
            dataframe = pd.json_normalize(header, 'analysis_results')
            if not dataframe.empty:
                del header['analysis_results']
                dataframe['latency_mean (ms)'] = dataframe['latency_mean']
                dataframe['cpu_usage (%)'] = dataframe['cpu_info_cpu_usage']
                dataframe['ru_maxrss'] = dataframe['sys_tracker_ru_maxrss']
                dataframe['T_experiment'] = dataframe['experiment_start'] / 1000000000
    except FileNotFoundError:
        print("Results for experiment " + filename + " do not exist")
    return header, dataframe


def getExperimentConfigs(yaml_experiments: list) -> 'list[PerfConfig]':
    accum = []
    for experiment in yaml_experiments:
        config_matrix = coerce_dict_vals_to_lists(experiment)
        config_combos = config_cartesian_product(config_matrix)
        perf_configs = [PerfConfig(**args) for args in config_combos]
        for config in perf_configs:
            if config not in accum:
                accum.append(config)
    return accum


def getExperimentLogPath(dir, cfg):
    return os.path.join(dir, cfg.log_file_name())


def coerce_to_list(val_or_list: object) -> list:
    if isinstance(val_or_list, list):
        return val_or_list
    return [val_or_list]


def coerce_dict_vals_to_lists(d: dict) -> dict:
    accum = {}
    for k in d:
        accum[k] = coerce_to_list(d[k])
    return accum


def config_cartesian_product(d: dict) -> list:
    accum = []
    keys, values = zip(*d.items())
    for bundle in itertools.product(*values):
        accum.append(dict(zip(keys, bundle)))
    return accum


def getDatasets(yaml_datasets: dict, log_dir) -> dict:
    dataset_configs = {}
    for dataset_id, dataset_details in yaml_datasets.items():
        perf_configs = getExperimentConfigs(dataset_details['experiments'])
        files = [pc.log_file_name() for pc in perf_configs]
        paths = [os.path.join(log_dir, f) for f in files]
        headers = []
        dataframes = []
        for path in paths:
            header, dataframe = parseLog(path)
            if not dataframe.empty:
                headers.append(header)
                dataframes.append(dataframe)
        dataset_configs[dataset_id] = DatasetConfig(
            name=dataset_details['name'],
            theme=dataset_details['theme'],
            headers=headers,
            dataframes=dataframes
        )
    return dataset_configs
