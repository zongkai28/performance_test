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

import plotly.graph_objects as go

from .utils import TraceConfig


def msg_size_vs_latency_cpu(configs: "list[TraceConfig]"):
    message_types = [
      'Array1k',
      'Array4k',
      'Array16k',
      'Array64k',
      'Array256k',
      'Array1m',
      'Array4m',
    ]

    results = []
    for config in configs:
        res = {
          'x': [],
          'y_lat': [],
          'y_cpu': []
        }
        for m, message_type in enumerate(message_types):
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

    fig_lat = go.Figure()

    # Plot latency vs message size
    fig_lat = go.FigureWidget()
    fig_lat.layout.title = 'Mean latency vs Message Size'
    fig_lat.layout.xaxis.title = 'Message Size'
    fig_lat.layout.xaxis.tickmode = 'array'
    fig_lat.layout.xaxis.tickvals = list(range(len(message_types)))
    fig_lat.layout.xaxis.ticktext = message_types
    fig_lat.layout.yaxis.title = 'Mean latency (ms)'
    fig_lat.layout.width = 1200
    fig_lat.layout.height = 800

    for result in results:
        fig_lat.add_scatter(x=result['x'],
                            y=result['y_lat'],
                            name=result['name'],
                            line=dict(color=result['color'], width=3, dash=result['dash']))

    fig_cpu = go.Figure()

    # Plot latency vs message size
    fig_cpu = go.FigureWidget()
    fig_cpu.layout.title = 'CPU Usage vs Message Size'
    fig_cpu.layout.xaxis.title = 'Message Size'
    fig_cpu.layout.xaxis.tickmode = 'array'
    fig_cpu.layout.xaxis.tickvals = list(range(len(message_types)))
    fig_cpu.layout.xaxis.ticktext = message_types
    fig_cpu.layout.yaxis.title = 'CPU Usage (%)'
    fig_cpu.layout.width = 1200
    fig_cpu.layout.height = 800

    for result in results:
        fig_cpu.add_scatter(x=result['x'],
                            y=result['y_cpu'],
                            name=result['name'],
                            line=dict(color=result['color'], width=3, dash=result['dash']))

    return fig_lat, fig_cpu


def time_vs_latency_cpu_mem(configs: "list[TraceConfig]"):
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
