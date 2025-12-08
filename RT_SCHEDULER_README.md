# Real-Time Scheduling Algorithms for Zephyr RTOS

A comprehensive implementation of 6 real-time scheduling algorithms for Zephyr, designed for research, education, and practical embedded systems development.

## Overview

This project extends Zephyr RTOS with a modular framework supporting multiple real-time scheduling algorithms:

| Algorithm | Type | Key Feature | Use Case |
|-----------|------|-------------|----------|
| **EDF** | Deadline-based | Optimal for single CPU | Hard real-time systems |
| **Weighted EDF** | Deadline + Weight | Task importance | Mixed-criticality systems |
| **WSRT** | Work + Weight | Response time optimization | Interactive systems |
| **RMS** | Static Priority | Predictable behavior | Safety-critical systems |
| **LLF** | Laxity-based | Early miss detection | Overload monitoring |
| **PFS** | Fairness-based | Starvation prevention | Mixed workloads |

## Quick Start

### 1. Configure Your Scheduler

Add to `prj.conf`:

```kconfig
# Choose ONE scheduler
CONFIG_736_MOD_EDF=y        # Weighted EDF (recommended)
# CONFIG_736_WSRT=y         # WSRT
# CONFIG_736_RMS=y          # RMS
# CONFIG_736_LLF=y          # LLF
# CONFIG_736_PFS=y          # PFS
# CONFIG_SCHED_DEADLINE=y   # Standard EDF

CONFIG_PRINTK=y
CONFIG_CONSOLE=y
CONFIG_TICKLESS_KERNEL=y
```

### 2. Write Your Application

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>

void my_task(void *p1, void *p2, void *p3)
{
    // Configure: 100ms period, 30ms execution, weight=2
    k_thread_rt_config(k_current_get(), 100, 30, 2);
    
    while (1) {
        k_sleep(K_MSEC(100));
        do_work();
    }
}
```

### 3. Build and Run

```bash
west build -b native_sim
west build -t run
```

**See [QUICK_START.md](QUICK_START.md) for detailed getting started guide.**

## Documentation

| Document | Description |
|----------|-------------|
| **[QUICK_START.md](QUICK_START.md)** | Get running in 5 minutes |
| **[SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md)** | Complete usage guide with examples |
| **[ALGORITHM_COMPARISON.md](ALGORITHM_COMPARISON.md)** | Detailed algorithm comparison |
| **[MODULAR_DESIGN.md](MODULAR_DESIGN.md)** | Implementation architecture |
| **[NEW_ALGORITHMS.md](NEW_ALGORITHMS.md)** | LLF and PFS deep dive |
| **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** | API and config cheat sheet |

## Features

### Modular Design
- Each algorithm in separate comparison function
- Easy to add new algorithms
- Clean separation of concerns
- Well-documented code

### Comprehensive API
```c
// Simple all-in-one configuration
k_thread_rt_config(tid, period_ms, exec_time_ms, weight);

// Or configure individually
k_thread_deadline_set(tid, deadline_cycles);
k_thread_exec_time_set(tid, exec_cycles);
k_thread_weight_set(tid, weight);
```

### Built-in Evaluation Framework
- Test applications in `app/simple_eval_step1/` and `app/advanced_eval/`
- Automated testing across all schedulers and workloads
- Python graphing scripts for visualization
- CSV data export for analysis

### Zero Runtime Overhead
- Compile-time algorithm selection
- No function pointers or dynamic dispatch
- Inline comparison functions
- Minimal memory footprint

## Testing and Evaluation

### Run Automated Tests

```bash
# Quick test
cd app/simple_eval_step1
west build -b native_sim
west build -t run

# Full evaluation (6 schedulers × 4 workloads = 24 tests)
bash scripts/run_all_tests.sh

# Generate comparison graphs
python3 scripts/generate_graphs.py

# View results
ls results/graphs/
```

### Test Results Include

- Response time statistics (avg, min, max, jitter)
- Deadline miss rates
- Context switch counts
- CPU utilization
- Per-task performance metrics

### Example Graphs Generated

- Response time by scheduler across workloads
- Deadline miss rate comparison
- Response time distribution histograms
- Scheduler performance heatmaps

## Architecture

### File Structure

```
kernel/
├── sched.c                    # Syscall implementations
├── Kconfig                    # Scheduler selection menu
└── include/
    └── priority_q.h          # Algorithm comparison functions

include/zephyr/kernel/
├── sched_rt.h                # Public RT scheduling API
└── thread.h                  # Thread structure (prio_weight, etc.)

doc/kernel/services/scheduling/
└── rt_schedulers.rst         # API documentation

app/
├── simple_eval_step1/        # Basic evaluation (50 activations)
├── advanced_eval/            # Advanced evaluation (100 activations)
└── samples/scheduler_example/  # Simple example application
```

### Algorithm Implementation

Each algorithm implements a comparison function:

```c
// Example: Weighted EDF
static ALWAYS_INLINE int32_t z_sched_cmp_weighted_edf(
    struct k_thread *t1, struct k_thread *t2)
{
    uint32_t d1 = t1->base.prio_deadline;
    uint32_t d2 = t2->base.prio_deadline;
    int w1 = t1->base.prio_weight;
    int w2 = t2->base.prio_weight;
    
    if (w1 == 0) w1 = 1;
    if (w2 == 0) w2 = 1;
    
    return (d2 / w2) - (d1 / w1);
}
```

All comparison functions are called from `z_sched_prio_cmp()` based on the selected configuration.

## Algorithm Comparison

### When to Use Each Scheduler

```
Need hard deadlines?
├─ Yes → All tasks equal importance?
│   ├─ Yes → EDF
│   └─ No → Weighted EDF
│
└─ No → Need fairness?
    ├─ Yes → PFS
    └─ No → Minimize response time?
        ├─ Yes → WSRT
        └─ No → Predictable static?
            ├─ Yes → RMS
            └─ No → Early miss detection? → LLF
```

### Performance Characteristics

| Scheduler | Complexity | Context Switches | Memory | Utilization Bound |
|-----------|-----------|------------------|---------|-------------------|
| EDF | O(1) | Low | Minimal | 100% |
| Weighted EDF | O(1) | Low | Minimal | <100% |
| WSRT | O(1) | Medium | +runtime tracking | Variable |
| RMS | O(1) | Low (static) | Minimal | 69% |
| LLF | O(1) | High | +runtime tracking | Variable |
| PFS | O(1) | Medium | +runtime tracking | N/A |

## Research Applications

This framework is suitable for:

1. **Comparative Studies**
   - Benchmark all 6 algorithms under identical workloads
   - Analyze performance vs. overhead tradeoffs
   - Study dynamic vs. static priority scheduling

2. **Algorithm Development**
   - Clean modular structure for adding new algorithms
   - Comprehensive test framework already in place
   - Automated data collection and visualization

3. **Mixed-Criticality Systems**
   - Compare weight-based prioritization strategies
   - Study graceful degradation under overload
   - Analyze fairness vs. deadline guarantees

4. **Education**
   - Hands-on implementation of classic RT algorithms
   - Real RTOS environment (not simulation)
   - Complete examples and documentation

## Development

### Adding a New Algorithm

1. **Add comparison function** to `kernel/include/priority_q.h`:
```c
static ALWAYS_INLINE int32_t z_sched_cmp_my_algo(
    struct k_thread *t1, struct k_thread *t2)
{
    // Your algorithm logic here
    return priority_difference;
}
```

2. **Add Kconfig option** to `kernel/Kconfig`:
```kconfig
config 736_MY_ALGO
    bool "My Algorithm"
    select 736
    help
      Description of your algorithm
```

3. **Add to comparison chain** in `z_sched_prio_cmp()`:
```c
#elif defined CONFIG_736_MY_ALGO
    return z_sched_cmp_my_algo(thread_1, thread_2);
```

4. **Update test scripts** to include new algorithm

See [MODULAR_DESIGN.md](MODULAR_DESIGN.md) for detailed architecture.

### Thread Attributes Available

```c
struct _thread_base {
    int8_t prio;              // Base priority level
    
#ifdef CONFIG_SCHED_DEADLINE
    int prio_deadline;        // Absolute deadline
#endif

#ifdef CONFIG_736
    int prio_exec_time;       // Expected execution time
    int prio_weight;          // Thread weight/importance
    
#ifdef CONFIG_736_TIME_LEFT
    int prio_time_left;       // Remaining execution time
#endif
#endif
};
```

## Examples

### Example 1: Simple Periodic Task

```c
void sensor_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 100, 20, 2);
    
    while (1) {
        k_sleep(K_MSEC(100));
        read_sensor();
    }
}
```

### Example 2: Mixed-Criticality System

```c
// Safety-critical task
void safety_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 20, 5, 10);  // High weight
    while (1) {
        k_sleep(K_MSEC(20));
        safety_check();
    }
}

// Background logging
void log_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 1000, 50, 1);  // Low weight
    while (1) {
        k_sleep(K_MSEC(1000));
        write_log();
    }
}
```

See [samples/scheduler_example/](samples/scheduler_example/) for complete examples.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Tasks not scheduled | Ensure all RT tasks have same base priority |
| Weights ignored | Check you're using a weighted scheduler (not plain EDF) |
| WSRT/LLF not working | Enable runtime tracking configs |
| High deadline misses | Verify utilization < 100%, reduce workload |

See [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) troubleshooting section for more details.

## License

This project extends Zephyr RTOS and follows the same licensing:
- SPDX-License-Identifier: Apache-2.0

## Contributing

This is a research project. Contributions welcome:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests and documentation
5. Submit a pull request

## References

### Classic Papers
- Liu & Layland (1973) - "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"
- Dertouzos (1974) - "Control Robotics: The Procedural Control of Physical Processes" (LLF)

### Implementation
- Zephyr RTOS Documentation: https://docs.zephyrproject.org/
- This implementation based on Zephyr commit: `72d4dced86d`

## Citation

If you use this work in your research, please cite:

```bibtex
@misc{zephyr-rt-schedulers,
  title={Real-Time Scheduling Algorithms for Zephyr RTOS},
  author={CS736 Project},
  year={2025},
  url={https://github.com/jostapei99/cs736-zephyr}
}
```

## Support

- **Documentation**: Start with [QUICK_START.md](QUICK_START.md)
- **Examples**: See `samples/scheduler_example/`
- **Evaluation**: Run `app/simple_eval_step1/scripts/run_all_tests.sh`
- **Issues**: Check [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) troubleshooting

## Roadmap

- [x] 6 core scheduling algorithms
- [x] Modular architecture
- [x] Comprehensive documentation
- [x] Automated testing framework
- [x] Graphing and analysis tools
- [ ] Multi-core scheduling support
- [ ] Runtime scheduler switching
- [ ] Additional metrics (energy consumption, etc.)
- [ ] Integration with Zephyr tracing infrastructure

---

**Ready to get started?** See [QUICK_START.md](QUICK_START.md) for a 5-minute introduction!

**Want to learn more?** Read [SCHEDULER_USER_GUIDE.md](SCHEDULER_USER_GUIDE.md) for comprehensive documentation!

**Need quick reference?** Check [QUICK_REFERENCE.md](QUICK_REFERENCE.md) for API cheat sheet!
