# Workload 3: Mixed Criticality System

## Overview
This workload simulates a mixed-criticality system where tasks have different levels of importance and deadline strictness. Common in safety-critical systems like avionics (DO-178C), medical devices (IEC 62304), and automotive (ISO 26262).

## Task Set with Criticality Levels

| Thread | Priority | Criticality | Period/Pattern | Execution | Deadline |
|--------|----------|-------------|----------------|-----------|----------|
| Safety Monitor | 0 | Safety-Critical | 10ms | 1ms | HARD (must NEVER miss) |
| Mission Function | 2 | Mission-Critical | 20ms | 5ms (15ms overload) | Firm (occasional miss OK) |
| User Interface | 4 | Non-Critical | 100ms | 8ms | Soft |
| Diagnostics | 6 | Best-Effort | Aperiodic | 10ms | None |

## Criticality Levels Explained

### Safety-Critical (Level A)
- **MUST NEVER** miss deadline
- Failure can cause catastrophic consequences
- Highest priority, guaranteed execution
- Example: Flight control, brake system monitoring

### Mission-Critical (Level B)
- Should rarely miss deadlines
- Occasional miss acceptable but tracked
- Degraded performance acceptable under overload
- Example: Navigation, radar processing

### Non-Critical (Level C)
- Soft real-time requirements
- Delayed execution acceptable
- Can be shed during overload
- Example: Display updates, user input

### Best-Effort (Level D)
- No deadline guarantees
- Runs when resources available
- First to be shed under load
- Example: Logging, diagnostics, health monitoring

## Test Scenarios

The workload simulates several stress conditions:

### 1. Normal Operation (0-5s)
- All tasks running normally
- System utilization: ~51%
- All deadlines should be met

### 2. Overload Condition (5-10s)
- Mission function execution time triples (5ms → 15ms)
- System utilization: ~143% (not schedulable!)
- Tests scheduler behavior under overload

### 3. Mode Change (7.5s)
- System transitions to degraded mode
- Low-priority tasks shed to ensure critical tasks meet deadlines
- Tests graceful degradation

### 4. Recovery (10s+)
- Return to normal operation
- System should stabilize

## Metrics Measured

1. **Safety Violation**: Any deadline miss by safety-critical task
2. **Mode Changes**: Number of system mode transitions
3. **Task Shedding**: How many times tasks were not executed due to mode
4. **Tardiness by Criticality**: Deadline miss rate per criticality level
5. **Graceful Degradation**: System behavior under overload

## Building and Running

```bash
west build -b qemu_cortex_m3 app/scheduler_evaluation/workload3_mixed_criticality
west build -t run
```

## Expected Results

### Safe Scheduler Behavior
```
Safety Monitor: 0 deadline misses ✓
Mission Function: Few deadline misses during overload (acceptable)
User Interface: Some deadline misses during overload (acceptable)
Diagnostics: Heavily shed during overload (expected)
```

### Unsafe Scheduler Behavior
```
Safety Monitor: >0 deadline misses ✗ CRITICAL FAILURE
```

## Schedulability Analysis

### Normal Mode
- U = 1/10 + 5/20 + 8/100 = 0.10 + 0.25 + 0.08 = 0.43 (43%) ✓ Schedulable

### Overload Mode
- U = 1/10 + 15/20 + 8/100 = 0.10 + 0.75 + 0.08 = 0.93 (93%)
- Over RMA bound for 3 tasks (0.78) → Not schedulable!
- Lower priority tasks will miss deadlines

### Degraded Mode (Shed Diagnostics)
- U = 1/10 + 15/20 = 0.10 + 0.75 = 0.85 (85%)
- Still challenging but critical tasks protected

## Interpretation

- **Zero safety task misses** = Correct scheduler behavior
- **Mode changes** = System correctly detecting and responding to overload
- **Selective shedding** = Proper priority enforcement
- **Graceful degradation** = System maintains critical functions under stress

## Real-World Applications

This workload models:
- Avionics flight control systems (DO-178C)
- Medical device embedded systems (IEC 62304)
- Automotive safety systems (ISO 26262)
- Industrial control with safety interlocks
- Any system with mixed-criticality requirements
