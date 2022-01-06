# performance_report

This package serves two purposes:

1. Run multiple `performance_test` experiments
2. Plot the combined results of those experiments

## Quick start

Install the required dependencies

```
# install bokeh and selenium
python3 -m pip install bokeh selenium
# install firefox-geckodriver
sudo apt install firefox-geckodriver
```

Note: all the commnds below are ran from the `colcon_ws` where `performance_test/performance_report` is installed.

```
# Build performance_test and performance_report
colcon build

# Set up the environment
source install/setup.bash

# Run perf_test for each experiment in the yaml file
ros2 run performance_report runner \
  --log-dir perf_logs \
  --test-name readme_experiment \
  --configs src/performance_test/performance_report/cfg/runner/apexos/run_one_experiment.yaml

# The runner generates log files to the specified directory: `./perf_logs/readme_experiement/`

# Generate the plots configured in the specified yaml file
ros2 run performance_report plotter \
  --log-dir perf_logs \
  --test-name readme_experiment \
  --configs src/performance_test/performance_report/cfg/plotter/apexos/plot_one_experiment.yaml

# The generated plots should be saved to the current directory where plotter was ran from

# Generate the reports configured in the specified yaml file
ros2 run performance_report reporter \
  --log-dir perf_logs \
  --test-name readme_experiment \
  --configs src/performance_test/performance_report/cfg/reporter/apexos/report_one_experiment.yaml
```

The above example only runs and plots one experiement, however multiple experiements can be ran
automatically by specifying it in the configuration file.

Try the above commands again, this time replacing the `yaml` file with `*_many_experiements.yaml`
for each executable.

When running many experiments the `runner` will by default skip any experiments that already have
log files generated in the specified `log_dir`. This can be overridden by adding the `-f` or
`--force` argument to the command.

See `performance_report/cfg` for more example yaml files and feel free to try them out yourself.

## Notes

- Currently, this tool is intended for ROS 2 with rmw_cyclone_dds, or Apex.OS with
  Apex.Middleware. It has not been tested with any other transport.
- If the run configuration includes `SHMEM` or `ZERO_COPY` transport, then a file for
  configuring the middleware will be created to enable the shared memory transfer.
  - You must start RouDi before running the experiments. This tool will not automatically
    start it for you.
