# RT Statistics Example

This sample demonstrates the use of kernel-level real-time scheduler statistics.

## Features

- Automatic tracking of activations, deadline misses, preemptions
- Response time statistics (min/max/avg/stddev/jitter)
- Waiting time and execution time tracking
- Detailed timestamps for fine-grained analysis
- Zero manual tracking overhead

## Building

```bash
west build -b native_sim samples/rt_stats_example
```

## Running

```bash
west build -t run
```

## Configuration

The sample uses:
- `CONFIG_736_RT_STATS` - Enable RT statistics
- `CONFIG_736_RT_STATS_DETAILED` - Track detailed timestamps
- `CONFIG_736_RT_STATS_SQUARED` - Enable variance/jitter calculation

## Output

The application prints:
1. CSV data for each activation (for graphing)
2. Periodic summaries every 10 activations
3. Final comprehensive statistics when complete

Example output:
```
╔══════════════════════════════════════════════════╗
║  Final Statistics for Task1 (High Priority)
╠══════════════════════════════════════════════════╣
║  Activations:             50
║  Deadline Misses:          0 (0.0%)
║  Context Switches:       102
║  Preemptions:             25
╠══════════════════════════════════════════════════╣
║  Response Time (ms):
║    Min:                   12
║    Max:                   28
║    Avg:                   18
║    Std Dev:                4
║    Jitter:                16
╚══════════════════════════════════════════════════╝
```

## Comparison with Manual Tracking

**Benefits of Kernel Statistics:**
- No application-level tracking code needed
- Lower overhead (integrated into scheduler)
- More accurate (captures true kernel behavior)
- Consistent across all applications
- Automatically tracks preemptions and context switches

**Manual Tracking:**
- Requires per-thread statistics structures
- Manual timestamp collection
- Risk of race conditions
- Cannot track scheduler-level events
