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

import numpy as np
import pandas as pd

from bokeh.models import ColumnDataSource, FactorRange
from bokeh.models.tools import HoverTool
from bokeh.models.widgets.markups import Div
from bokeh.models.widgets.tables import DataTable, TableColumn
from bokeh.palettes import cividis
from bokeh.plotting import figure

from .msg import MSG_TYPES
from .utils import DatasetConfig

result = {
    'x': [],
    'y_lat': [],
    'y_cpu': []
}


def generateFigure(figConfig, datasets: "list[DatasetConfig]"):
    # time series, normal range
    fig = None
    is_categorical = False
    if figConfig['x_range'] == 'T_experiment':
        fig = figure(
                title=figConfig['title'],
                x_axis_label=figConfig['x_axis_label'],
                y_axis_label=figConfig['y_axis_label'],
                plot_width=figConfig['size']['width'],
                plot_height=figConfig['size']['height'],
                margin=(10, 10, 10, 10)
        )
    else:
        # assume catagorical if not a time series
        is_categorical = True
        fig = figure(
                name=figConfig['name'],
                title=figConfig['title'],
                x_axis_label=figConfig['x_axis_label'],
                y_axis_label=figConfig['y_axis_label'],
                x_range=FactorRange(),
                plot_width=figConfig['size']['width'],
                plot_height=figConfig['size']['height'],
                margin=(10, 10, 10, 10)
        )
    for dataset_name in figConfig['datasets']:
        dataset = datasets[dataset_name]
        df = dataset.dataframe
        df_summary = df.describe().reset_index()
        is_wildcard = False
        # filter dataframe based on specified ranges
        if(len(dataset.experiments) > 1):
            # if multiple experiments in dataset
            filtered_results = []
            for experiment in dataset.experiments:
                exp_df = experiment.as_dataframe()
                exp_cols = list(exp_df.columns.values)
                matching_rows = dataset.dataframe[exp_cols].stack().isin(exp_df.stack().values).unstack()
                result_df = dataset.dataframe[matching_rows.all(axis='columns')]
                if(exp_df['wildcard'].values[0]):
                    fig = add_line_to_figure(
                        experiment.log_file_name().split('.')[0],
                        fig,
                        figConfig,
                        is_categorical,
                        dataset,
                        result_df)
                    is_wildcard = True
                else:
                    # default to average of specified range
                    summary_df = result_df.groupby(experiment.get_members()).mean().reset_index()
                    filtered_results.append(summary_df)
            if not is_wildcard:
                df = pd.concat(filtered_results, ignore_index=True)
        if not is_wildcard:
            fig = add_line_to_figure(
                dataset.name,
                fig,
                figConfig,
                is_categorical,
                dataset,
                df)
        # add hover tool
        hover = HoverTool()
        hover.tooltips = [
            (figConfig['y_axis_label'], '@{' + figConfig['y_range'] + '}{0.0000} ms'),
        ]
        fig.add_tools(hover)
    return fig


def add_line_to_figure(line_name, fig, figConfig, is_categorical, dataset, df):
    scatter_name = line_name + ' ' + dataset.theme.marker.shape
    if is_categorical:
        fig.x_range.factors = list(df[figConfig['x_range']])
        source = ColumnDataSource(df)
        fig.scatter(
            name=scatter_name,
            x=figConfig['x_range'],
            y=figConfig['y_range'],
            source=source,
            marker=dataset.theme.marker.shape,
            size=dataset.theme.marker.size,
            fill_color=dataset.theme.color
        )
        fig.line(
            name=line_name,
            x=figConfig['x_range'],
            y=figConfig['y_range'],
            source=source,
            line_color=dataset.theme.color,
            line_width=dataset.theme.line.width,
            line_alpha=dataset.theme.line.alpha,
            legend_label=line_name,
        )
    else:
        fig.scatter(
            name=scatter_name,
            x=figConfig['x_range'],
            y=figConfig['y_range'],
            source=df,
            marker=dataset.theme.marker.shape,
            size=dataset.theme.marker.size,
            fill_color=dataset.theme.color
        )
        fig.line(
            name=line_name,
            x=figConfig['x_range'],
            y=figConfig['y_range'],
            source=df,
            line_color=dataset.theme.color,
            line_width=dataset.theme.line.width,
            line_alpha=dataset.theme.line.alpha,
            legend_label=line_name,
        )
    return fig


def msg_size_vs_latency_cpu(configs: "list[DatasetConfig]"):
    results = []
    for config in configs:
        res = result.copy()
        for m, message_type in enumerate(MSG_TYPES):
            for i, header in enumerate(config.headers):
                if header['msg_name'] == message_type:
                    df = config.dataframes[i]
                    res['x'].append(m)
                    res['y_lat'].append(np.mean(df['latency_mean (ms)']))
                    res['y_cpu'].append(np.mean(df['cpu_usage (%)']))
                    res['name'] = config.name
                    res['color'] = config.color
                    res['dash'] = config.dash
        results.append(res)

    fig_lat = figure()

    # Plot latency vs message size
    #    fig_lat = go.FigureWidget()
    #    fig_lat.layout.title = 'Mean latency vs Message Size'
    #    fig_lat.layout.xaxis.title = 'Message Size'
    #    fig_lat.layout.xaxis.tickmode = 'array'
    #    fig_lat.layout.xaxis.tickvals = list(range(len(message_types)))
    #    fig_lat.layout.xaxis.ticktext = message_types
    #    fig_lat.layout.yaxis.title = 'Mean latency (ms)'
    #    fig_lat.layout.width = 1200
    #    fig_lat.layout.height = 800
    #
    #    for result in results:
    #        fig_lat.add_scatter(x=result['x'],
    #                            y=result['y_lat'],
    #                            name=result['name'],
    #                            line=dict(color=result['color'], width=3, dash=result['dash']))

    fig_cpu = figure()

    # Plot latency vs message size
    #    fig_cpu = go.FigureWidget()
    #    fig_cpu.layout.title = 'CPU Usage vs Message Size'
    #    fig_cpu.layout.xaxis.title = 'Message Size'
    #    fig_cpu.layout.xaxis.tickmode = 'array'
    #    fig_cpu.layout.xaxis.tickvals = list(range(len(message_types)))
    #    fig_cpu.layout.xaxis.ticktext = message_types
    #    fig_cpu.layout.yaxis.title = 'CPU Usage (%)'
    #    fig_cpu.layout.width = 1200
    #    fig_cpu.layout.height = 800
    #
    #    for result in results:
    #        fig_cpu.add_scatter(x=result['x'],
    #                            y=result['y_cpu'],
    #                            name=result['name'],
    #                            line=dict(color=result['color'], width=3, dash=result['dash']))

    return fig_lat, fig_cpu


def time_vs_latency_cpu_mem(configs: "list[DatasetConfig]"):
    for config in configs:
        if len(config.headers) != 1:
            raise ValueError('Time trace should have exactly one experiment')

    fig_lat = go.FigureWidget()
    fig_lat.layout.title = 'Latency mean'
    fig_lat.layout.xaxis.title = 'Time (s)'
    fig_lat.layout.yaxis.title = 'Mean latency (ms)'
    fig_lat.layout.width = 1200
    fig_lat.layout.height = 800

    for tc in configs:
        fig_lat.add_scatter(x=tc.dataframes[0]['T_experiment'],
                            y=tc.dataframes[0]['latency_mean (ms)'],
                            name=tc.name,
                            line=dict(color=tc.color, width=3, dash=tc.dash))

    fig_cpu = go.FigureWidget()
    fig_cpu.layout.title = 'CPU Usage'
    fig_cpu.layout.xaxis.title = 'Time (s)'
    fig_cpu.layout.yaxis.title = 'CPU Usage (%)'
    fig_cpu.layout.width = 1200
    fig_cpu.layout.height = 800

    for tc in configs:
        fig_cpu.add_scatter(x=tc.dataframes[0]['T_experiment'],
                            y=tc.dataframes[0]['cpu_usage (%)'],
                            name=tc.name,
                            line=dict(color=tc.color, width=3, dash=tc.dash))

    fig_mem = go.FigureWidget()
    fig_mem.layout.title = 'Memory consumption'
    fig_mem.layout.xaxis.title = 'Time (s)'
    fig_mem.layout.yaxis.title = 'Memory consumption (MB)'
    fig_mem.layout.width = 1200
    fig_mem.layout.height = 800

    for tc in configs:
        fig_mem.add_scatter(x=tc.dataframes[0]['T_experiment'],
                            y=tc.dataframes[0]['ru_maxrss'],
                            name=tc.name,
                            line=dict(color=tc.color, width=3, dash=tc.dash))

    return fig_lat, fig_cpu, fig_mem
