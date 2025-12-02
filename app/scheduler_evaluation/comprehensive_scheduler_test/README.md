# Comprehensive Scheduler Evaluation Test

## Overview

This application provides a **unified, comprehensive test suite** that evaluates ALL Zephyr schedulers using a single integrated workload. It combines the best aspects of all 6 individual workloads into one comprehensive test.

## What It Tests

The application runs **6 test phases** that evaluate different aspects of scheduler performance:

### Phase 1: Periodic Task Test
- 4 periodic tasks with different periods (10ms, 20ms, 50ms, 100ms)
- Measures deadline misses and response times
- Tests basic priority-based scheduling

### Phase 2: Event-Driven Test
- 3 event handlers with different priorities
- Measures event response latency
- Tests priority handling and preemption

### Phase 3: Scalability Test
- Runs with 1, 5, 10, and 15 threads
- Measures throughput (iterations/second)
- **Shows O(N) vs O(log N) vs O(1) differences**

### Phase 4: Priority Inversion Test
- Tests mutex behavior
- Measures priority inversion duration
- Shows if priority inheritance is working

### Phase 5: Overload Stress Test
- Runs under normal load then overload
- Measures deadline misses and tardiness
- Tests scheduler behavior under stress

### Phase 6: EDF Deadline Scheduling Test
- Tests earliest-deadline-first scheduling (if enabled)
- 3 tasks with different deadline requirements
- Only runs if CONFIG_SCHED_DEADLINE=y

## Schedulers Evaluated

### 1. SCHED_SIMPLE (O(N) list)
```kconfig
CONFIG_SCHED_SIMPLE=y
```
- **Best for**: Few threads (<10)
- **Pros**: Smallest code size (~2KB savings)
- **Cons**: Performance degrades with thread count

### 2. SCHED_SCALABLE (O(log N) red-black tree)
```kconfig
CONFIG_SCHED_SIMPLE is not set
CONFIG_SCHED_SCALABLE=y
```
- **Best for**: Many threads (>20)
- **Pros**: Scales well with thread count
- **Cons**: ~2KB code overhead

### 3. SCHED_MULTIQ (O(1) array of lists)
```kconfig
CONFIG_SCHED_SIMPLE is not set
CONFIG_SCHED_MULTIQ=y
```
- **Best for**: Traditional RTOS behavior
- **Pros**: Constant time operations, predictable
- **Cons**: Higher RAM usage, incompatible with deadline scheduling

### 4. SCHED_DEADLINE (EDF)
```kconfig
CONFIG_SCHED_DEADLINE=y
CONFIG_SCHED_SIMPLE=y
```
- **Best for**: Tasks with explicit deadlines
- **Pros**: Optimal single-core scheduling, can achieve 100% utilization
- **Cons**: Must use with SIMPLE scheduler

## Quick Start

### Test SIMPLE scheduler:
```bash
cd /home/jack/cs736-project/zephyr
west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run | tee results_simple.txt
```

### Test SCALABLE scheduler:
```bash
# Edit prj.conf
sed -i 's/CONFIG_SCHED_SIMPLE=y/# CONFIG_SCHED_SIMPLE is not set/' app/scheduler_evaluation/comprehensive_scheduler_test/prj.conf
echo "CONFIG_SCHED_SCALABLE=y" >> app/scheduler_evaluation/comprehensive_scheduler_test/prj.conf

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run | tee results_scalable.txt
```

### Test MULTIQ scheduler:
```bash
# Edit prj.conf
sed -i 's/CONFIG_SCHED_SCALABLE=y/# CONFIG_SCHED_SCALABLE is not set/' app/scheduler_evaluation/comprehensive_scheduler_test/prj.conf
echo "CONFIG_SCHED_MULTIQ=y" >> app/scheduler_evaluation/comprehensive_scheduler_test/prj.conf

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run | tee results_multiq.txt
```

### Test DEADLINE scheduler:
```bash
# Restore SIMPLE and add DEADLINE
cat > app/scheduler_evaluation/comprehensive_scheduler_test/prj.conf << 'EOF'
# Comprehensive Scheduler Test Configuration
CONFIG_TIMING_FUNCTIONS=y
CONFIG_PRINTK=y
CONFIG_EARLY_CONSOLE=y
CONFIG_MAIN_STACK_SIZE=8192
CONFIG_THREAD_NAME=y
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000
CONFIG_SCHED_SIMPLE=y
CONFIG_SCHED_DEADLINE=y
CONFIG_NUM_COOP_PRIORITIES=0
CONFIG_NUM_PREEMPT_PRIORITIES=32
CONFIG_TIMESLICING=n
CONFIG_MUTEX=y
CONFIG_SEMAPHORE=y
EOF

west build -p -b qemu_cortex_m3 app/scheduler_evaluation/comprehensive_scheduler_test
timeout 35 west build -t run | tee results_deadline.txt
```

## Comparing Results

### Key Metrics to Compare

**Phase 1 - Periodic Tasks:**
- Deadline misses (should be 0 under normal load)
- Max response time

**Phase 2 - Event-Driven:**
- Max latency (lower is better)
- Event distribution across priorities

**Phase 3 - Scalability:**
- Throughput at 1, 5, 10, 15 threads
- **This shows the O(N) vs O(log N) vs O(1) difference!**

**Phase 4 - Priority Inversion:**
- Inversion duration (should be <6ms with priority inheritance)

**Phase 5 - Overload:**
- Which tasks miss deadlines
- Max tardiness
- Recovery behavior

**Phase 6 - Deadline (EDF):**
- Deadline miss rate with explicit deadlines

### Expected Results

**SIMPLE Scheduler:**
- Good for phases 1, 2, 4, 5 with few threads
- **Phase 3 will show degrading performance as thread count increases**
- Works with EDF (Phase 6)

**SCALABLE Scheduler:**
- Good overall performance
- **Phase 3 shows better scaling than SIMPLE**
- Does NOT work with EDF

**MULTIQ Scheduler:**
- Best real-time predictability
- **Phase 3 shows constant performance regardless of thread count**
- Does NOT work with EDF

**DEADLINE Scheduler:**
- Only Phase 6 shows the difference
- Should have 0 deadline misses even at high utilization

## Automated Testing

Use the provided script `test_all_schedulers.sh` to run all tests automatically.

## Output Format

Each phase prints:
1. Phase description
2. Test execution
3. Results summary

Final summary compares all phases.

## Use Cases

**Choose this test when you need to:**
1. Evaluate which scheduler is best for your application
2. Compare all schedulers with the same workload
3. Understand scheduler behavior under various conditions
4. Validate scheduler configuration changes
5. Benchmark scheduler performance

## Advantages Over Individual Workloads

- **Single test** covers all scenarios
- **Direct comparison** with same workload
- **Faster evaluation** (30 seconds vs 2 minutes per workload)
- **Unified metrics** across all tests
- **Automatic scheduler detection** in output

## Technical Details

- **Total Duration**: ~30 seconds
- **Memory**: ~17KB FLASH, ~39KB RAM
- **Threads**: Up to 26 total across all phases
- **Timing**: Uses k_cycle_get_32() for QEMU compatibility
- **Synchronization**: Mutexes, semaphores (built-in)

## Interpreting Results

### If you see:
- **High deadline misses in Phase 1**: System overloaded or scheduler not suitable
- **High latency in Phase 2**: Scheduler overhead or priority inversion
- **Degrading Phase 3 throughput**: O(N) scheduler with too many threads
- **Long inversion in Phase 4**: Priority inheritance not working
- **Many misses in Phase 5**: Normal under overload, compare which tasks miss
- **Misses in Phase 6**: Utilization too high or EDF not working

### Recommendations:
- **<10 threads, code size critical**: Use SIMPLE
- **>20 threads, dynamic workload**: Use SCALABLE
- **Real-time, deterministic**: Use MULTIQ
- **Explicit deadlines**: Use DEADLINE with SIMPLE

## Advanced Usage

### Custom Configuration

Edit `prj.conf` to:
- Enable/disable priority inheritance
- Change thread priorities
- Adjust timing resolution
- Enable thread runtime stats

### Port to Different Board

```bash
west build -p -b <your_board> app/scheduler_evaluation/comprehensive_scheduler_test
```

Make sure the board supports timing functions.

## Related Workloads

This comprehensive test combines elements from:
- Workload 1: Periodic tasks
- Workload 2: Event-driven
- Workload 3: Mixed criticality
- Workload 4: Deadline scheduling
- Workload 5: Scalability
- Workload 6: Overload stress

For detailed analysis of specific scenarios, see the individual workloads.
