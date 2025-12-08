# New Algorithms: LLF and PFS

## Summary

Two new scheduling algorithms have been added to create a comprehensive suite:

1. **Least Laxity First (LLF)** - Dynamic priority based on slack time
2. **Proportional Fair Scheduling (PFS)** - Fairness-oriented scheduling

This brings the total to **6 algorithms** covering different scheduling paradigms.

---

## 1. Least Laxity First (LLF)

### Overview
LLF is a dynamic priority algorithm that schedules based on **laxity** (slack time):

```
Laxity = Deadline - Remaining_Execution_Time - Current_Time
```

Threads with **lower laxity** (tighter deadlines relative to remaining work) get **higher priority**.

### Why LLF?

**Theoretical Importance:**
- Classic real-time scheduling algorithm (alongside EDF and RMS)
- Optimal under certain conditions
- Well-studied in real-time literature

**Research Value:**
1. **Dynamic vs. Static**: Compare with RMS (static) and EDF (semi-dynamic)
2. **Early Miss Detection**: LLF can detect imminent misses earlier than EDF
3. **Thrashing Behavior**: Known to exhibit thrashing under overload - valuable for research
4. **Laxity Metric**: Study laxity as a scheduling criterion vs. deadlines alone

**Practical Insights:**
- Shows when dynamic priorities help/hurt
- Demonstrates overhead of frequent priority recalculation
- Illustrates tradeoff between optimality and complexity

### Implementation

```c
static ALWAYS_INLINE int32_t z_sched_cmp_llf(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t d1 = t1->base.prio_deadline;
    uint32_t d2 = t2->base.prio_deadline;
    uint32_t tl1 = t1->base.prio_time_left;
    uint32_t tl2 = t2->base.prio_time_left;

    int32_t laxity1 = (int32_t)(d1 - tl1);
    int32_t laxity2 = (int32_t)(d2 - tl2);

    return laxity2 - laxity1;  // Lower laxity = higher priority
}
```

### Configuration

```kconfig
CONFIG_736_LLF=y
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
```

Requires runtime tracking to maintain `time_left`.

### Expected Behavior

**Light Load:**
- Similar to EDF (plenty of slack time)
- Few priority changes

**Medium Load:**
- More dynamic than EDF
- Priority changes as tasks execute
- May show increased context switches

**Heavy Load:**
- Frequent priority updates
- Early detection of potential misses
- Can exhibit thrashing (rapid context switches)

**Overload:**
- Severe thrashing possible
- Negative laxity indicates guaranteed miss
- Interesting for overload research

### Research Questions

1. How much overhead does dynamic priority add?
2. When does laxity-based scheduling outperform EDF?
3. What is the thrashing threshold?
4. Can we predict/prevent thrashing?
5. How effective is early miss detection?

---

## 2. Proportional Fair Scheduling (PFS)

### Overview
PFS ensures **fairness** in CPU allocation based on weights:

```
Priority = virtual_runtime / weight
```

Threads with the **lowest** weighted virtual runtime are scheduled first.

### Why PFS?

**Theoretical Importance:**
- Foundation of modern schedulers (Linux CFS)
- Well-studied fairness metric
- Prevents starvation

**Research Value:**
1. **Fairness vs. Deadlines**: Contrast with deadline-based algorithms
2. **Starvation Prevention**: Show long-term fairness guarantees
3. **Mixed Workloads**: Study RT + best-effort task interaction
4. **Weight Effectiveness**: Compare weight mechanisms across algorithms

**Practical Insights:**
- When fairness matters more than deadlines
- Long-running system behavior
- Mixed criticality without hard deadlines

### Implementation

```c
static ALWAYS_INLINE int32_t z_sched_cmp_pfs(struct k_thread *t1, struct k_thread *t2)
{
    uint32_t runtime1 = t1->base.prio_exec_time;
    uint32_t runtime2 = t2->base.prio_exec_time;
    int w1 = t1->base.prio_weight;
    int w2 = t2->base.prio_weight;

    if (w1 == 0) w1 = 1;
    if (w2 == 0) w2 = 1;

    return (runtime1 / w1) - (runtime2 / w2);
}
```

Note: In this simplified version, `prio_exec_time` is repurposed as accumulated runtime.

### Configuration

```kconfig
CONFIG_736_PFS=y
```

Minimal requirements - uses existing weight infrastructure.

### Expected Behavior

**Light Load:**
- Smooth CPU sharing
- Low response time variance
- Predictable allocation

**Medium Load:**
- Maintains fairness proportions
- May miss deadlines if present
- Prevents any thread starvation

**Heavy Load:**
- Equal (weighted) suffering
- No priority inversion
- Graceful degradation

**Overload:**
- Continues fair allocation
- Ignores deadlines completely
- All threads make progress (slowly)

### Research Questions

1. How does fairness affect deadline miss rates?
2. What's the overhead of fairness tracking?
3. When is PFS better than deadline-based scheduling?
4. Can we combine fairness with soft deadlines?
5. How do weights affect CPU share distribution?

---

## Algorithm Portfolio Summary

### Coverage Matrix

| Dimension | Algorithms |
|-----------|------------|
| **Deadline-based** | EDF, Weighted EDF, LLF |
| **Work-based** | WSRT, LLF |
| **Fairness-based** | PFS |
| **Static Priority** | RMS |
| **Dynamic Priority** | LLF, WSRT, PFS |
| **Weight-aware** | Weighted EDF, WSRT, PFS |

### Comparison Dimensions

With 6 algorithms, you can study:

1. **Static vs. Dynamic**
   - RMS (static) vs. EDF (semi-dynamic) vs. LLF (fully dynamic)

2. **Deadline vs. Fairness**
   - EDF/LLF vs. PFS

3. **Weighted Approaches**
   - Weighted EDF (deadline-centric)
   - WSRT (work-centric)
   - PFS (fairness-centric)

4. **Optimality Tradeoffs**
   - EDF (optimal), RMS (optimal static), LLF (optimal under conditions)
   - Weighted EDF (loses optimality for flexibility)

5. **Overhead Analysis**
   - EDF (lowest) → RMS → Weighted EDF → WSRT → PFS → LLF (highest)

6. **Overload Behavior**
   - EDF: domino effect
   - Weighted EDF: controlled degradation
   - LLF: thrashing
   - PFS: fair degradation
   - RMS: priority-based degradation
   - WSRT: starvation possible

---

## Evaluation Strategy

### Recommended Test Matrix

**Workloads:** (already have)
- LIGHT (50%)
- MEDIUM (70%)
- HEAVY (90%)
- OVERLOAD (110%)

**Algorithms:** (now 6)
- EDF, Weighted EDF, WSRT, RMS, LLF, PFS

**Total Tests:** 4 workloads × 6 algorithms = **24 test configurations**

### Key Metrics

1. **Deadline Miss Rate** - Critical for RT algorithms
2. **Response Time** - Average and variance
3. **Context Switches** - Overhead indicator
4. **Fairness Index** - CPU share distribution (Jain's fairness)
5. **Thrashing Detection** - For LLF analysis
6. **Starvation Events** - For PFS validation

### Interesting Comparisons

**Same Workload Across Algorithms:**
```
HEAVY workload:
├─ EDF vs. LLF → Dynamic priority benefit?
├─ EDF vs. Weighted EDF → Value of task importance?
├─ WSRT vs. LLF → Work vs. laxity?
├─ RMS vs. All → Static vs. dynamic?
└─ PFS vs. All → Fairness cost?
```

**Same Algorithm Across Workloads:**
```
LLF across loads:
├─ LIGHT → Normal operation
├─ MEDIUM → Dynamic benefits emerge
├─ HEAVY → Thrashing starts
└─ OVERLOAD → Severe thrashing
```

---

## Implementation Status

**Core Implementation** - Complete
- Comparison functions added to `priority_q.h`
- Kconfig options added
- Documentation updated

**Evaluation Support** - Complete
- Test scripts updated (6 schedulers)
- Graphing scripts updated (6 colors)
- Color scheme extended

**Documentation** - Complete
- Algorithm descriptions
- Configuration guide
- Comparison table
- Research questions

**Next Steps** - Optional
1. Run full evaluation (24 test configs)
2. Generate comparison graphs
3. Analyze results
4. Write findings document

---

## Research Contributions

With these 6 algorithms, you can publish research on:

1. **"Comprehensive Comparison of RT Scheduling in Embedded RTOS"**
   - Compare all 6 algorithms empirically
   - Identify when each works best
   - Provide algorithm selection guidelines

2. **"Dynamic Priority Overhead in Real-Time Systems"**
   - Focus on EDF vs. LLF vs. WSRT
   - Quantify dynamic priority costs
   - Study thrashing behavior

3. **"Fairness vs. Deadlines in Real-Time Scheduling"**
   - PFS vs. deadline-based algorithms
   - Mixed workload analysis
   - Starvation prevention

4. **"Weight-Based Prioritization Strategies"**
   - Compare Weighted EDF, WSRT, PFS
   - Different weight interpretations
   - Mixed-criticality applications

---

## Conclusion

**LLF** adds:
- Dynamic priority insights
- Thrashing behavior for research
- Early miss detection capability
- Laxity as a scheduling metric

**PFS** adds:
- Fairness perspective
- Starvation prevention
- Mixed workload support
- Non-deadline-based scheduling

Together, they complete a comprehensive algorithm suite covering:
- Classic algorithms (EDF, RMS)  
- Modern extensions (Weighted EDF, WSRT, PFS)  
- Dynamic priorities (LLF, WSRT, PFS)  
- Multiple scheduling paradigms (deadline, work, fairness)

This gives you a **publication-ready** scheduler comparison framework!
