# Understanding Zero Latency Values

## What You're Seeing

If you're getting zero (or very small) latency values, this could actually be **correct behavior**! Here's why:

## How Latency is Calculated

In the workloads, latency is calculated as:

```c
int64_t latency_ticks = actual_wakeup - next_wakeup;
uint64_t latency_us = k_ticks_to_us_ceil64(latency_ticks);
```

This measures: **How late did the thread wake up compared to when it should have woken up?**

## Why You Might Get Zero Latency

### 1. Perfect Scheduling (Ideal Case)
If the scheduler is working perfectly and there's no contention:
- Thread sleeps until time `T`
- Thread wakes up exactly at time `T`
- Latency = `T - T = 0` ← **This is actually good!**

### 2. Tick Resolution Too Coarse
If `CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000`:
- 1 tick = 100 microseconds
- If latency is 50us, it rounds down to 0 ticks
- Solution: Check the debug output added - it shows actual tick values

### 3. QEMU Timing Issues
QEMU's timer emulation may not be accurate:
- Virtual CPU runs at unpredictable speed
- Timer interrupts may be batched
- Solution: Test on real hardware for accurate results

## How to Verify What's Happening

I added debug output to workload 1. When you run it, you'll see:

```
Timing calibration: XXXXXXX cycles per second
Cycles per microsecond: XXX

[Sensor] Exec 0: next=100, actual=100, latency_ticks=0, latency_us=0
[Sensor] Exec 1: next=200, actual=202, latency_ticks=2, latency_us=200
[Sensor] Exec 2: next=300, actual=305, latency_ticks=5, latency_us=500
...
```

### Interpreting the Output

**If `total_cycles = 0`**:
```
ERROR: Timing subsystem not working!
```
→ The timing API isn't supported on your platform (QEMU issue)
→ Response time measurements will be zero
→ **But tick-based latency should still work!**

**If `cycles_per_us = 0`** but `total_cycles > 0`:
```
WARNING: cycles_per_us is zero! Using fallback value.
```
→ CPU is running <1MHz (unlikely)
→ Fallback to 1 cycle/us used

**If latency_ticks = 0 but latency_us > 0**:
→ Impossible! Should not happen.

**If latency_ticks = 0 AND latency_us = 0**:
→ Thread woke up exactly on time! (Perfect scheduling)
→ Or tick resolution is too coarse to measure small latencies

## What Zero Latency Actually Means

### In QEMU:
- Threads may wake up exactly on time because there's no real hardware interference
- No real interrupts, no real bus contention, no real I/O delays
- **Zero latency in QEMU doesn't mean zero latency on real hardware**

### On Real Hardware:
- You would see non-zero latency due to:
  - Interrupt latency
  - Scheduler overhead
  - Cache misses
  - Bus contention
  - Higher priority interrupts

## Solutions

### 1. Increase Load (Recommended)
Make threads compete more:
- Increase number of threads
- Increase work duration (EXEC_US values)
- Add more priority levels

Example - edit `src/main.c`:
```c
#define SENSOR_EXEC_US 5000  // Was 2000 - increase workload
#define CONTROL_EXEC_US 10000 // Was 5000
```

### 2. Increase Tick Rate
Edit `prj.conf`:
```
CONFIG_SYS_CLOCK_TICKS_PER_SEC=100000  # Was 10000
```
Now 1 tick = 10 microseconds (better resolution)

### 3. Test on Real Hardware
QEMU timing is not realistic. Test on actual Cortex-M hardware:
```bash
west build -p -b <your_board> app/scheduler_evaluation/workload1_periodic_control
west flash
# Monitor serial output
```

### 4. Use Response Time Instead
Response time is measured with the timing API (cycle counter):
- More accurate than tick-based latency
- Shows actual execution time
- Should never be zero if timing API works

Look at these stats instead:
```
Average response time: XXX us  ← This uses cycle counter
Max response time: XXX us
```

## Quick Check

Run workload 1 and look at the output:

### Good (Timing Works):
```
Timing calibration: 12000000 cycles per second
Cycles per microsecond: 12

Sensor Statistics:
  Average response time: 2150 us  ← Non-zero, good!
  Average latency: 0 us           ← Zero is OK if response time works
```

### Bad (Timing Broken):
```
Timing calibration: 0 cycles per second
Cycles per microsecond: 1

Sensor Statistics:
  Average response time: 0 us     ← Zero, timing API not working
  Average latency: 0 us           ← Zero, but tick-based should still work
```

If timing API doesn't work, the **latency** measurement (tick-based) should still be valid. Check the debug output to see actual tick values.

## Bottom Line

**Zero latency is not necessarily a bug!** It could mean:

✅ Scheduler is working perfectly (in QEMU's idealized environment)  
✅ Tick resolution is too coarse to measure small delays  
✅ Workload is too light (threads not competing)  

**To get meaningful latency measurements:**

1. Run on real hardware, not QEMU
2. Increase workload (more threads, longer execution times)
3. Increase tick rate for better resolution
4. Focus on response time if timing API works

**The debug output I added will show you exactly what's happening with the tick values.**
