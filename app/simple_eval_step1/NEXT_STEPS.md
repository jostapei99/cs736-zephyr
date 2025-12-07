# Next Steps for Two-Task System

## Current Configuration
- **Task1**: Period=100ms, Exec=60ms, Deadline=90ms (60% util)
- **Task2**: Period=200ms, Exec=100ms, Deadline=190ms (50% util)
- **Total Utilization**: 110% (OVERLOADED!)
- **Both tasks**: Priority = 5 (same)

## Recommended Progression

### 🔬 Experiment 1: Observe Overload Behavior

**Run the current setup** for 50 activations and observe:

```bash
cd /home/jack/cs736-project/zephyr/app/simple_eval_step1
west build -t run | tee results_overload.log
```

**Questions to answer:**
1. Which task misses more deadlines?
2. What's the pattern - every Nth activation, or random?
3. Do the tasks interfere with each other?
4. What are the actual response times vs. expected?

**Prediction**: With 110% utilization, at least one task MUST miss deadlines.

---

### ✅ Experiment 2: Make It Schedulable

**Change to 90% utilization:**

```c
#define TASK1_EXEC_TIME_MS 50  // Was 60
#define TASK2_EXEC_TIME_MS 80  // Was 100
```

**Expected result**: ALL deadlines should be met

**If deadlines still miss**, investigate:
- Is `k_busy_wait()` accurate?
- Are there scheduling delays?
- Is printk() adding overhead?

---

### 🎯 Experiment 3: Test Priority Impact

Keep 110% utilization, but change priorities:

**Test A**: Task1 higher priority
```c
// In main():
k_thread_create(&task1_thread, ..., 4, ...);  // Higher priority
k_thread_create(&task2_thread, ..., 6, ...);  // Lower priority
```

**Test B**: Task2 higher priority  
```c
k_thread_create(&task1_thread, ..., 6, ...);  // Lower priority
k_thread_create(&task2_thread, ..., 4, ...);  // Higher priority
```

**Question**: Does the higher-priority task always meet its deadline?

---

### ⚡ Experiment 4: Enable EDF Scheduler

EDF (Earliest Deadline First) should schedule based on deadlines, not static priority.

**Step 1**: Verify EDF is enabled in `prj.conf`:
```properties
CONFIG_SCHED_DEADLINE=y  # Already set
```

**Step 2**: Set task deadlines via API (add to each task function):

```c
void task1(void *a, void *b, void *c)
{
    k_tid_t me = k_current_get();
    
    // Try to set deadline (may need kernel support)
    // struct k_thread_runtime_stats stats;
    // k_thread_runtime_stats_get(me, &stats);
    
    init_release(&task1_stats, TASK1_PERIOD_MS);
    
    while (1) {
        // ... existing code
    }
}
```

**Step 3**: Check if EDF changes which task misses deadlines

**Expected**: Task with tighter deadline (Task1: 90ms) should be prioritized

---

### 📊 Experiment 5: Add CSV Output

Export data for analysis:

```c
// Add at top of file:
#define CSV_OUTPUT 1

// In each task, after calculating response_time:
#if CSV_OUTPUT
printk("CSV,%u,%d,%u,%u,%d\n",
       (uint32_t)now,        // timestamp
       1,                     // task_id
       response_time,         // response_time
       deadline_met,          // deadline_met
       (deadline_met ? 0 : (int)(end - abs_deadline))  // lateness
);
#endif
```

Then analyze with:
```bash
west build -t run > data.log
grep "^CSV" data.log > metrics.csv
```

---

### 🔧 Experiment 6: Add Third Task

Make it more interesting:

```c
#define TASK3_PERIOD_MS    300
#define TASK3_EXEC_TIME_MS 40
#define TASK3_DEADLINE_MS  300

K_THREAD_STACK_DEFINE(task3_stack, STACK_SIZE);
struct k_thread task3_thread;
task_stats_t task3_stats = {0};

// Copy task1/task2 structure for task3
```

**New utilization**: 60% + 50% + 13% = 123% (even more overloaded!)

**Questions**:
- Does Task3 ever run?
- Which tasks starve?
- How does priority ordering affect it?

---

### 🎓 Experiment 7: Systematic Workload Testing

Create different workload profiles:

```c
// Light load (50% util) - all should succeed
TASK1: Period=100, Exec=30
TASK2: Period=200, Exec=40

// Medium load (70% util) - should succeed
TASK1: Period=100, Exec=40
TASK2: Period=200, Exec=60

// Heavy load (95% util) - tight but schedulable
TASK1: Period=100, Exec=55
TASK2: Period=200, Exec=80

// Overload (110% util) - must fail
TASK1: Period=100, Exec=60
TASK2: Period=200, Exec=100
```

Test each and document:
- Deadline miss rate
- Which task fails first
- Average response time
- Jitter (variation)

---

## Quick Wins to Try Now

### 1. **See current behavior** (2 minutes)
```bash
west build -t run | head -200
```
Look at first 20 activations - any deadline misses?

### 2. **Reduce to 90% util** (1 minute)
```c
#define TASK1_EXEC_TIME_MS 50
#define TASK2_EXEC_TIME_MS 80
```
Rebuild and verify all deadlines met.

### 3. **Try different priorities** (3 minutes)
Change priority values in `main()`, see if it matters.

### 4. **Turn off debug prints** (1 minute)
```c
#define DEBUG_STATEMENTS 0
```
See if printk overhead was causing deadline misses.

---

## What I'd Recommend Next

**Start with Experiment 1**: Run your current overloaded system and observe which task suffers. This will teach you about scheduler behavior under load.

**Then do Experiment 2**: Make it schedulable and verify your measurement is accurate.

**Then Experiment 3**: Test if priority matters in Zephyr's default scheduler.

**Finally Experiment 4**: Enable proper EDF and see if it helps.

Want help with any of these? Let me know which experiment interests you most!
