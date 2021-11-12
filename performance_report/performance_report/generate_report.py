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
import pandas as pd
import yaml


from bokeh.embed import components
from bokeh.models import ColumnDataSource, FactorRange
from bokeh.models.tools import HoverTool
from bokeh.models.widgets.markups import Div
from bokeh.models.widgets.tables import DataTable, TableColumn
from bokeh.palettes import cividis
from bokeh.plotting import figure

from jinja2 import Environment, PackageLoader, select_autoescape
from .logs import getDatasets
from .utils import create_dir, PerfArgParser


def generateReports(report_cfg_file, output_dir):
    html_figures = []

    env = Environment(
        loader=PackageLoader('performance_report'),
        autoescape=select_autoescape()
    )
    with open(report_cfg_file, "r") as f:
        reports_cfg = yaml.load(f, Loader=yaml.FullLoader)
        datasets = getDatasets(reports_cfg["datasets"], output_dir)
        try:
            for report in reports_cfg['reports']:
                print('[REPORT] ' + report)
                for fig in reports_cfg['reports'][report]['figures']:
                    # time series, normal range
                    plot = None
                    is_categorical = False
                    if fig['x_range'] == 'T_experiment':
                        plot = figure(
                                title=fig['title'],
                                x_axis_label=fig['x_axis_label'],
                                y_axis_label=fig['y_axis_label'],
                                plot_width=fig['size']['width'],
                                plot_height=fig['size']['height'],
                                margin=(10, 10, 10, 10)
                        )
                    else:
                        # assume catagorical if not a time series
                        is_categorical = True
                        plot = figure(
                                title=fig['title'],
                                x_axis_label=fig['x_axis_label'],
                                y_axis_label=fig['y_axis_label'],
                                x_range=FactorRange(),
                                plot_width=fig['size']['width'],
                                plot_height=fig['size']['height'],
                                margin=(10, 10, 10, 10)
                        )
                    for dataset_name in fig['datasets']:
                        print('[DATASET] \t' + dataset_name)
                        dataset = datasets[dataset_name]
                        df = dataset.dataframe
                        df_summary = df.describe().reset_index()
                        
                        # filter dataframe based on specified ranges
                        if(len(dataset.experiments) > 1):
                            # if multiple experiments in dataset
                            filtered_results = []
                            for experiment in dataset.experiments:
                                exp_df = experiment.as_dataframe()
                                exp_cols = list(exp_df.columns.values)
                                matching_rows = dataset.dataframe[exp_cols].stack().isin(exp_df.stack().values).unstack()
                                result_df = dataset.dataframe[matching_rows.all(axis='columns')]
                                # default to average of specified range
                                summary_df = result_df.groupby(experiment.get_members()).mean().reset_index()
                                filtered_results.append(summary_df)
                            df = pd.concat(filtered_results, ignore_index=True)
                        line_name = dataset.name
                        scatter_name = line_name + ' ' + dataset.theme.marker.shape

                        if is_categorical:
                            plot.x_range.factors = list(df[fig['x_range']])
                            source = ColumnDataSource(df)
                            plot.scatter(
                                name=scatter_name,
                                x=fig['x_range'],
                                y=fig['y_range'],
                                source=source,
                                marker=dataset.theme.marker.shape,
                                size=dataset.theme.marker.size,
                                fill_color=dataset.theme.color
                            )
                            plot.line(
                                name=line_name,
                                x=fig['x_range'],
                                y=fig['y_range'],
                                source=source,
                                line_color=dataset.theme.color,
                                line_width=dataset.theme.line.width,
                                line_alpha=dataset.theme.line.alpha,
                                legend_label=line_name,
                            )
                        else:
                            plot.scatter(
                                name=scatter_name,
                                x=fig['x_range'],
                                y=fig['y_range'],
                                source=df,
                                marker=dataset.theme.marker.shape,
                                size=dataset.theme.marker.size,
                                fill_color=dataset.theme.color
                            )
                            plot.line(
                                name=line_name,
                                x=fig['x_range'],
                                y=fig['y_range'],
                                source=df,
                                line_color=dataset.theme.color,
                                line_width=dataset.theme.line.width,
                                line_alpha=dataset.theme.line.alpha,
                                legend_label=line_name,
                            )
                        # add hover tool
                        hover = HoverTool()
                        hover.tooltips = [
                            (fig['y_axis_label'], '@{' + fig['y_range'] + '}{0.0000} ms'),
                        ]
                        plot.add_tools(hover)
                        script, div = components(plot)
                        html_figures.append((script, div))
                for html_fig in html_figures:
                    template = env.get_template('test.html')
                    output = template.render(
                        script=str(html_fig[0]),
                        div=str(html_fig[1]))
                    report_title = reports_cfg['reports'][report]['report_title']
                    with open(report_title + '.html', 'w') as result:
                        result.write(output)
        except KeyError:
            print("No time_vs_latency_cpu_mem plots specified in given trace yaml file")


def main():
    parser = PerfArgParser()
    parser.init_args()
    args = parser.parse_args()
    log_dir = getattr(args, "log_dir")
    test_name = getattr(args, "test_name")
    report_cfg_files = getattr(args, "configs")

    log_dir = os.path.join(log_dir, test_name)
    for report_cfg_file in report_cfg_files:
        generateReports(report_cfg_file, log_dir)


if __name__ == "__main__":
    main()
