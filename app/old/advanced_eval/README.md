# Advanced RT Scheduler Evaluation

## Overview

This application enhances `simple_eval_step1` with advanced features for comprehensive real-time scheduler evaluation.

## Key Enhancements

### 1. **Jitter Analysis**
- Calculates response time variance and standard deviation
- Tracks jitter metrics for predictability analysis
- Displays jitter in summary reports

### 2. **Dynamic Workload Support**
- Runtime workload selection via shell commands
- Ability to change workload parameters on-the-fly
- Phase-based testing (multiple workloads in one run)

### 3. **Extended Metrics**
- Context switch counting
- CPU utilization tracking
- Preemption statistics
- Task interference analysis

### 4. **Advanced Output Options**
- JSON output format (in addition to CSV)
- Real-time progress updates
- Configurable verbosity levels
- Statistical confidence intervals

### 5. **Resource Monitoring**
- Thread stack usage tracking
- Memory consumption analysis
- Overhead measurements

### 6. **Arrival Time Jitter Simulation**
- Simulate realistic task arrival patterns
- Configurable jitter distributions
- Test scheduler robustness

## Building

```bash
cd /home/jack/cs736-project/zephyr
west build -b native_sim app/advanced_eval
west build -t run
```

## Configuration

Edit `include/workloads.h` to configure workloads.
Edit `prj.conf` to select scheduler.

## Features vs simple_eval_step1

| Feature | simple_eval_step1 | advanced_eval |
|---------|------------------|---------------|
| Basic RT tasks | ✓ | ✓ |
| CSV output | ✓ | ✓ |
| Deadline tracking | ✓ | ✓ |
| Response time stats | ✓ | ✓ |
| Jitter calculation | Partial | ✓ Complete |
| JSON output | ✗ | ✓ |
| Dynamic workloads | ✗ | ✓ |
| Context switch tracking | ✗ | ✓ |
| CPU utilization | ✗ | ✓ |
| Stack usage | ✗ | ✓ |
| Arrival jitter | ✗ | ✓ |
| Confidence intervals | ✗ | ✓ |
| Shell commands | ✗ | ✓ |

## Usage

See `GUIDE.md` for detailed usage instructions.
