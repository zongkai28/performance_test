# performance_report

This package serves two purposes:

1. Run multiple `performance_test` experiments
2. Plot the combined results of those experiments

## Quick start

```
# Build performance_test and performance_report
colcon build

# Set up the environment
source install/setup.bash

# Run perf_test for each experiment in the yaml file
ros2 run performance_report runner \
  --log-dir perf_logs \
  --test-name readme_experiment \
  --configs src/performance_test/performance_report/cfg/runs/one_experiment.yaml

# Generate the configured plots in the yaml file
ros2 run performance_report plotter \
  --log-dir perf_logs \
  --test-name readme_experiment \
  --configs src/performance_test/performance_report/cfg/plots/one_experiment.yaml

# The perf_test log files, and the generated plots, are in the perf_logs directory
```

See `performance_report/cfg` for example yaml files.

## Notes

- Currently, this tool is intended for ROS 2 with rmw_cyclone_dds, or Apex.OS with
  Apex.Middleware. It has not been tested with any other transport.
- If the run configuration includes `SHMEM` or `ZERO_COPY` transport, then a file for
  configuring the middleware will be created to enable the shared memory transfer.
  - You must start RouDi before running the experiments. This tool will not automatically
    start it for you.
