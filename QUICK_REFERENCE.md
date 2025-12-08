# RT Scheduler Quick Reference

## 6 Algorithms at a Glance

| # | Algorithm | Priority Formula | When to Use |
|---|-----------|-----------------|-------------|
| 1 | **EDF** | `deadline` | Hard RT, optimal utilization |
| 2 | **Weighted EDF** | `deadline/weight` | Mixed criticality |
| 3 | **WSRT** | `time_left/weight` | Minimize response time |
| 4 | **RMS** | `exec_time` (shorter first) | Predictable static priority |
| 5 | **LLF** | `deadline - time_left` (laxity) | Early miss detection |
| 6 | **PFS** | `runtime/weight` (fairness) | Prevent starvation |

## Configuration Cheat Sheet

```kconfig
# Pick ONE:
CONFIG_SCHED_DEADLINE=y        # EDF (baseline)
CONFIG_736_MOD_EDF=y           # Weighted EDF
CONFIG_736_WSRT=y              # WSRT (needs runtime stats)
CONFIG_736_RMS=y               # RMS
CONFIG_736_LLF=y               # LLF (needs runtime stats)
CONFIG_736_PFS=y               # PFS

# If using WSRT or LLF, also add:
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y
```

## API Usage

```c
#include <zephyr/kernel/sched_rt.h>

// Simple way (all-in-one):
k_thread_rt_config(tid, period_ms, exec_time_ms, weight);

// Or individually:
k_thread_deadline_set(tid, k_ms_to_cyc_ceil32(100));
k_thread_exec_time_set(tid, k_ms_to_cyc_ceil32(50));
k_thread_weight_set(tid, 2);
```

## Running Tests

```bash
cd app/simple_eval_step1
bash scripts/run_all_tests.sh    # 6 schedulers × 4 workloads = 24 tests
python3 scripts/generate_graphs.py
```

Results in `app/simple_eval_step1/results/graphs/`

## Algorithm Decision Tree

```
┌─ Need hard deadlines?
│  ├─ Yes ─┬─ All tasks equally important?
│  │       ├─ Yes → EDF
│  │       └─ No ─┬─ Want dynamic priorities?
│  │              ├─ Yes → LLF
│  │              └─ No → Weighted EDF
│  │
│  └─ No ──┬─ Minimize response time?
│          ├─ Yes → WSRT
│          └─ No ─┬─ Need static priorities?
│                 ├─ Yes → RMS
│                 └─ No → PFS (fairness)
```

## Files Modified

- `kernel/include/priority_q.h` - Algorithm comparison functions
- `kernel/Kconfig` - Algorithm selection menu
- `app/*/scripts/run_all_tests.sh` - Added LLF, PFS to test matrix
- `app/*/scripts/generate_graphs.py` - Added colors for new algorithms

## Files Created

- `include/zephyr/kernel/sched_rt.h` - Public API
- `doc/kernel/services/scheduling/rt_schedulers.rst` - Documentation
- `samples/scheduler_example/` - Example application
- `MODULAR_DESIGN.md` - Architecture explanation
- `ALGORITHM_COMPARISON.md` - Detailed comparison
- `NEW_ALGORITHMS.md` - LLF & PFS explanation
- `QUICK_REFERENCE.md` - This file

## Key Differences

| Feature | EDF | Weighted EDF | WSRT | RMS | LLF | PFS |
|---------|-----|--------------|------|-----|-----|-----|
| Static Priority | Semi | Semi | No | Yes | No | No |
| Uses Weight | No | Yes | Yes | No | No | Yes |
| Uses Deadline | Yes | Yes | No | No | Yes | No |
| Tracks Runtime | No | No | Yes | No | Yes | Yes |
| Fairness Focus | No | No | No | No | No | Yes |
| Can Thrash | No | No | Maybe | No | Yes | No |

## Research Value

1. **Static vs Dynamic**: Compare RMS (static), EDF (semi), LLF/WSRT/PFS (dynamic)
2. **Weight Mechanisms**: Compare Weighted EDF, WSRT, PFS weight interpretations
3. **Overload Behavior**: Study EDF domino vs LLF thrashing vs PFS fairness
4. **Optimality Tradeoffs**: EDF optimal → Weighted EDF flexible → others specialized
5. **Overhead Analysis**: Measure context switches, runtime tracking cost

## Next Steps

1. [DONE] Algorithms implemented (6 total)
2. [DONE] Test scripts updated
3. [DONE] Documentation complete
4. [TODO] Run evaluation (24 test configs)
5. [TODO] Generate graphs
6. [TODO] Analyze results
7. [TODO] Write paper

## Support

- See `ALGORITHM_COMPARISON.md` for detailed algorithm info
- See `MODULAR_DESIGN.md` for implementation architecture
- See `NEW_ALGORITHMS.md` for LLF & PFS specifics
- See `doc/kernel/services/scheduling/rt_schedulers.rst` for full docs
