# RT Scheduling Algorithm Comparison

## Overview

This document provides a comprehensive comparison of all 6 real-time scheduling algorithms implemented in this branch.

## Algorithm Summary Table

| Algorithm | Type | Priority Basis | Key Metric | Dynamic? | Requires Time Tracking? |
|-----------|------|---------------|------------|----------|------------------------|
| **EDF** | Deadline-based | Absolute deadline | Earliest deadline | No | No |
| **Weighted EDF** | Deadline + Weight | deadline/weight | Weighted deadline | No | No |
| **WSRT** | Work-based + Weight | time_left/weight | Weighted remaining work | Yes | Yes |
| **RMS** | Period-based | Execution time (period proxy) | Shorter period/exec time | No | No |
| **LLF** | Laxity-based | deadline - time_left | Slack time | Yes | Yes |
| **PFS** | Fairness-based | virtual_runtime/weight | Fair CPU share | Yes | No |

## Detailed Comparison

### 1. Earliest Deadline First (EDF)
```
Priority: Earlier deadline = Higher priority
Formula: deadline₁ - deadline₂
```

**Strengths:**
- Optimal for single-processor systems (Liu & Layland)
- Simple to understand and implement
- Predictable behavior
- Low overhead

**Weaknesses:**
- No task differentiation (all deadlines equal)
- Domino effect under overload
- No fairness guarantees

**Best For:**
- Hard real-time systems
- When all tasks have equal importance
- Baseline comparison

---

### 2. Weighted EDF
```
Priority: Lower deadline/weight = Higher priority
Formula: (deadline₁/weight₁) - (deadline₂/weight₂)
```

**Strengths:**
- Task importance differentiation
- Still predictable like EDF
- Allows critical tasks to preempt less critical ones

**Weaknesses:**
- Weight selection requires domain knowledge
- Can starve low-weight tasks under heavy load
- Loses EDF optimality guarantees

**Best For:**
- Mixed-criticality systems
- When tasks have different importance levels
- Graceful degradation under overload

---

### 3. Weighted Shortest Remaining Time (WSRT)
```
Priority: Lower time_left/weight = Higher priority
Formula: (time_left₁/weight₁) - (time_left₂/weight₂)
```

**Strengths:**
- Optimizes response time
- Adapts to actual execution progress
- Weight-based prioritization

**Weaknesses:**
- High overhead (runtime tracking)
- Can starve long-running tasks
- Requires accurate execution time estimates

**Best For:**
- Minimizing average response time
- Systems with variable execution times
- When actual progress matters more than deadlines

---

### 4. Rate Monotonic Scheduling (RMS)
```
Priority: Shorter execution time = Higher priority
Formula: exec_time₂ - exec_time₁
```

**Strengths:**
- Static priorities (predictable)
- Well-studied theoretical properties
- Optimal for static priority scheduling
- Low runtime overhead

**Weaknesses:**
- Not optimal for dynamic priority
- Requires fixed periods
- Lower utilization bound (69% vs 100% for EDF)

**Best For:**
- Periodic task systems
- When predictability > utilization
- Safety-critical systems (static analysis friendly)

---

### 5. Least Laxity First (LLF)
```
Priority: Lower laxity = Higher priority
Laxity: deadline - time_left - current_time
Formula: laxity₂ - laxity₁
```

**Strengths:**
- Early detection of deadline misses
- Optimal in some scenarios
- Dynamic adaptation to execution progress
- Good overload handling (detects problems early)

**Weaknesses:**
- Can cause thrashing (many context switches)
- High overhead (frequent priority recalculation)
- Negative laxity indicates guaranteed deadline miss

**Best For:**
- Systems needing early miss detection
- Studying dynamic priority behavior
- Comparing with EDF under varying loads
- Research on thrashing behavior

---

### 6. Proportional Fair Scheduling (PFS)
```
Priority: Lower virtual_runtime/weight = Higher priority
Formula: (runtime₁/weight₁) - (runtime₂/weight₂)
```

**Strengths:**
- Fairness guarantees
- Starvation prevention
- Smooth resource allocation
- Works well with mixed workloads

**Weaknesses:**
- No hard deadline guarantees
- Not suitable for hard real-time
- Requires runtime accounting

**Best For:**
- Mixed RT + best-effort workloads
- Preventing starvation
- Long-running systems
- Fairness-critical applications

## Complexity Analysis

| Algorithm | Space | Time (per comparison) | Context Switches |
|-----------|-------|---------------------|------------------|
| EDF | O(1) | O(1) | Low |
| Weighted EDF | O(1) | O(1) | Low |
| WSRT | O(1) + runtime tracking | O(1) | Medium |
| RMS | O(1) | O(1) | Low (static) |
| LLF | O(1) + runtime tracking | O(1) | High |
| PFS | O(1) + runtime tracking | O(1) | Medium |

## Use Case Decision Tree

```
Need hard deadlines?
├─ Yes → Need task differentiation?
│   ├─ Yes → Weighted EDF
│   └─ No → EDF
│
└─ No → Need fairness?
    ├─ Yes → PFS
    └─ No → Minimize response time?
        ├─ Yes → WSRT
        └─ No → Static priorities?
            ├─ Yes → RMS
            └─ No → Early miss detection?
                ├─ Yes → LLF
                └─ No → EDF
```

## Research Questions Each Algorithm Addresses

### EDF
- Baseline performance for deadline-based scheduling
- Optimal utilization bounds
- Behavior under overload

### Weighted EDF
- Impact of task importance on scheduling
- Graceful degradation strategies
- Mixed-criticality scheduling

### WSRT
- Response time optimization
- Impact of remaining work vs. deadlines
- Adaptive scheduling based on progress

### RMS
- Static vs. dynamic priority comparison
- Predictability vs. utilization tradeoffs
- Classical real-time theory validation

### LLF
- Dynamic priority adaptation benefits/costs
- Early deadline miss detection
- Thrashing behavior under overload
- Laxity as a scheduling metric

### PFS
- Fairness in real-time systems
- Starvation prevention mechanisms
- Mixed workload performance
- Long-term resource allocation

## Experimental Insights

### Expected Performance Patterns

**Light Load (50% utilization):**
- All algorithms should meet all deadlines
- PFS may have higher response time variance
- LLF may show more context switches

**Medium Load (70% utilization):**
- Differentiation starts to appear
- Weighted algorithms prioritize high-weight tasks
- LLF begins showing dynamic behavior

**Heavy Load (90% utilization):**
- Clear performance differences
- EDF may start missing deadlines
- Weighted EDF protects high-weight tasks
- LLF shows thrashing
- PFS maintains fairness at cost of deadlines

**Overload (110% utilization):**
- All deadline-based algorithms miss some deadlines
- Weighted EDF: controlled degradation
- LLF: severe thrashing possible
- PFS: continues fair allocation, ignores deadlines
- RMS: misses based on static priorities

### Interesting Comparisons

1. **EDF vs. Weighted EDF**: Cost/benefit of task importance
2. **EDF vs. LLF**: Static vs. dynamic deadline-based scheduling
3. **WSRT vs. LLF**: Remaining work vs. laxity
4. **RMS vs. EDF**: Static vs. dynamic priorities
5. **PFS vs. all**: Fairness vs. deadline guarantees
6. **Weighted EDF vs. WSRT**: Deadline-centric vs. work-centric

## Configuration Dependencies

| Algorithm | Requires | Optional |
|-----------|----------|----------|
| EDF | `CONFIG_SCHED_DEADLINE` | - |
| Weighted EDF | `CONFIG_736`, `CONFIG_SCHED_DEADLINE` | - |
| WSRT | `CONFIG_736`, `CONFIG_736_TIME_LEFT`, `CONFIG_SCHED_THREAD_USAGE` | - |
| RMS | `CONFIG_736`, `CONFIG_SCHED_DEADLINE` | - |
| LLF | `CONFIG_736`, `CONFIG_736_TIME_LEFT`, `CONFIG_SCHED_THREAD_USAGE` | - |
| PFS | `CONFIG_736`, `CONFIG_SCHED_DEADLINE` | - |

## Thread Attributes Used

| Algorithm | deadline | exec_time | weight | time_left |
|-----------|----------|-----------|--------|-----------|
| EDF | x | - | - | - |
| Weighted EDF | x | - | x | - |
| WSRT | - | - | x | x |
| RMS | - | x | - | - |
| LLF | x | - | - | x |
| PFS | - | x (as runtime) | x | - |

## Evaluation Metrics

When comparing algorithms, measure:

1. **Deadline Miss Rate**: Percentage of missed deadlines
2. **Response Time**: Average, min, max, variance
3. **Context Switches**: Number of preemptions
4. **Jitter**: Response time variance
5. **Fairness**: CPU allocation proportional to weights
6. **Predictability**: Variance in behavior
7. **Overhead**: Runtime cost of scheduling decisions

## Conclusion

This diverse set of 6 algorithms provides:

- **Classic algorithms**: EDF, RMS (baseline, well-studied)
- **Weight-based**: Weighted EDF, WSRT, PFS (task differentiation)
- **Dynamic**: LLF, WSRT, PFS (adaptive behavior)
- **Deadline-focused**: EDF, Weighted EDF, LLF (hard real-time)
- **Fairness-focused**: PFS (starvation prevention)
- **Work-aware**: WSRT, LLF (execution progress tracking)

Together, they enable comprehensive research into:
- Static vs. dynamic priority scheduling
- Deadline vs. fairness tradeoffs
- Task importance mechanisms
- Overload behavior
- Scheduling overhead
- Mixed-criticality systems
