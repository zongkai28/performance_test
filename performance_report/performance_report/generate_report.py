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

from jinja2 import Environment, FileSystemLoader, select_autoescape
from .figures import generateFigure
from .logs import getDatasets
from .utils import create_dir, PerfArgParser


def generateReports(report_cfg_file, output_dir):
    cfg_dir, _ = os.path.split(report_cfg_file)
    html_figures = {}
    with open(report_cfg_file, "r") as f:
        reports_cfg = yaml.load(f, Loader=yaml.FullLoader)
        datasets = getDatasets(reports_cfg["datasets"], output_dir)
        try:
            for report_name, report_cfg in reports_cfg['reports'].items():
                for fig in report_cfg['figures']:
                    plot = generateFigure(fig, datasets)
                    script, div = components(plot)
                    html_figures[fig['name']] = script + div

                # fill in template
                template_dir, template_file = os.path.split(report_cfg['template_name'])
                loader_dir = os.path.join(cfg_dir, template_dir)
                env = Environment(
                    loader=FileSystemLoader(loader_dir),
                    autoescape=select_autoescape()
                )
                template = env.get_template(template_file)

                report_title = report_cfg['report_title']
                output = template.render(html_figures)
                    
                with open(report_title + '.html', 'w') as result:
                    result.write(output)
        except KeyError:
            print("Oops, something is wrong with the provided report configuration file....exiting")


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
