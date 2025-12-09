# RT Statistics Quick Start Guide

## Quick Test

```bash
cd /home/jack/cs736-project/zephyr
west build -b native_sim app/test_stats -p
west build -t run
```

## Test All Schedulers

```bash
cd /home/jack/cs736-project/zephyr/app/test_stats
./test_all_schedulers.sh
# Select option 1 to test all schedulers
```

## Manual Testing

### Weighted EDF
```bash
west build -b native_sim app/test_stats -p -- -DCONFIG_736_MOD_EDF=y
west build -t run
```

### RMS
```bash
west build -b native_sim app/test_stats -p -- -DCONFIG_736_RMS=y
west build -t run
```

### WSRT
```bash
west build -b native_sim app/test_stats -p -- -DCONFIG_736_WSRT=y
west build -t run
```

### LLF
```bash
west build -b native_sim app/test_stats -p -- -DCONFIG_736_LLF=y
west build -t run
```

### PFS
```bash
west build -b native_sim app/test_stats -p -- -DCONFIG_736_PFS=y
west build -t run
```

## What Gets Tested

1. ✓ Statistics collection (activations, preemptions, context switches)
2. ✓ Response time tracking (min/max/average)
3. ✓ Waiting time measurement
4. ✓ Deadline miss detection
5. ✓ Statistics reset functionality
6. ✓ Statistical accuracy validation
7. ✓ Variance/jitter calculation (if CONFIG_736_RT_STATS_SQUARED=y)

## Expected Output

```
╔════════════════════════════════════════╗
║  RT Statistics Test Application       ║
╚════════════════════════════════════════╝

Configuration:
  Scheduler:       Weighted EDF
  Test threads:    3
  Iterations:      10
  Workload size:   5000
  RT Stats:        ENABLED
  Detailed Stats:  ENABLED
  Variance Calc:   ENABLED

====================================
Test 1: Basic Statistics Collection
====================================
...
✓ Test 1 PASSED

====================================
Test 2: Statistics Reset
====================================
...
✓ Test 2 PASSED

====================================
Test 3: Statistics Accuracy
====================================
...
✓ Test 3 PASSED

====================================
Scheduler Performance Summary
====================================
...

╔════════════════════════════════════════╗
║  ALL TESTS PASSED ✓                   ║
╚════════════════════════════════════════╝
```

## Troubleshooting

### Build Errors

**Error: CONFIG_736_RT_STATS not defined**
- Solution: Ensure you're using kernel/Kconfig with RT stats options

**Error: k_thread_rt_stats_get not found**
- Solution: Make sure `CONFIG_736_ADD_ONS=y` is set

### Runtime Issues

**No statistics collected (all zeros)**
- Check: `CONFIG_736_RT_STATS=y` in prj.conf
- Check: One of the custom schedulers is enabled
- Check: Threads are actually running

**Timestamps always zero**
- Enable: `CONFIG_736_RT_STATS_DETAILED=y`

## Configuration Matrix

| Feature | Required Config | Description |
|---------|----------------|-------------|
| Basic stats | `CONFIG_736_RT_STATS=y` | Counters and timing |
| Timestamps | `CONFIG_736_RT_STATS_DETAILED=y` | Event timestamps |
| Variance | `CONFIG_736_RT_STATS_SQUARED=y` | Jitter calculation |
| Custom scheduler | One of `CONFIG_736_*=y` | Which algorithm to test |

## Files

- `src/main.c` - Main test application
- `prj.conf` - Default configuration
- `CMakeLists.txt` - Build configuration
- `README.md` - Full documentation
- `QUICK_START.md` - This file
- `test_all_schedulers.sh` - Automated test script
