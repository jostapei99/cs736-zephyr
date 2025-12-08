# RT Scheduler Quick Start Guide

Get up and running with real-time schedulers in 5 minutes!

## Step 1: Choose Your Scheduler (2 minutes)

Edit `prj.conf` and select **ONE** scheduler:

```kconfig
# Option 1: Standard EDF (baseline, no weights)
CONFIG_SCHED_DEADLINE=y

# Option 2: Weighted EDF (task importance)
# CONFIG_736_MOD_EDF=y

# Option 3: WSRT (minimize response time) - needs runtime stats
# CONFIG_736_WSRT=y
# CONFIG_SCHED_THREAD_USAGE=y
# CONFIG_THREAD_RUNTIME_STATS=y

# Option 4: RMS (static priority)
# CONFIG_736_RMS=y

# Option 5: LLF (early miss detection) - needs runtime stats
# CONFIG_736_LLF=y
# CONFIG_SCHED_THREAD_USAGE=y
# CONFIG_THREAD_RUNTIME_STATS=y

# Option 6: PFS (fairness)
# CONFIG_736_PFS=y

# Required for all
CONFIG_PRINTK=y
CONFIG_CONSOLE=y
CONFIG_TICKLESS_KERNEL=y
```

**Quick selector:**
- **Hard deadlines, all equal** → EDF
- **Hard deadlines, different importance** → Weighted EDF
- **Fast response time** → WSRT
- **Predictable & safe** → RMS
- **Detect overload early** → LLF
- **Fair CPU sharing** → PFS

## Step 2: Write Your Code (2 minutes)

```c
#include <zephyr/kernel.h>
#include <zephyr/kernel/sched_rt.h>
#include <zephyr/sys/printk.h>

void my_task(void *p1, void *p2, void *p3)
{
    // Configure: period=100ms, exec_time=30ms, weight=2
    k_thread_rt_config(k_current_get(), 100, 30, 2);
    
    while (1) {
        k_sleep(K_MSEC(100));  // Wait for next period
        
        // Do your work here
        printk("Task running!\n");
    }
}

#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(task_stack, STACK_SIZE);
struct k_thread task_thread;

int main(void)
{
    printk("Starting RT task...\n");
    
    k_thread_create(&task_thread, task_stack, STACK_SIZE,
                    my_task, NULL, NULL, NULL,
                    5,  // Priority
                    0, K_NO_WAIT);
    
    return 0;
}
```

## Step 3: Build and Run (1 minute)

```bash
west build -b native_sim
west build -t run
```

**That's it!** Your task is now running with real-time scheduling.

---

## Want Multiple Tasks?

```c
void fast_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 50, 10, 3);  // High weight
    while (1) {
        k_sleep(K_MSEC(50));
        printk("Fast task\n");
    }
}

void slow_task(void *p1, void *p2, void *p3)
{
    k_thread_rt_config(k_current_get(), 200, 30, 1);  // Low weight
    while (1) {
        k_sleep(K_MSEC(200));
        printk("Slow task\n");
    }
}

// Create both tasks with same priority (5)
// The scheduler will differentiate them based on your algorithm choice
```

---

## Understanding k_thread_rt_config()

```c
k_thread_rt_config(thread_id, period_ms, exec_time_ms, weight);
```

- **period_ms**: How often the task runs (e.g., 100ms = 10Hz)
- **exec_time_ms**: How long it takes to run (e.g., 30ms)
- **weight**: Task importance (higher = more important)
  - Weighted EDF: Higher weight = effectively earlier deadline
  - WSRT/PFS: Higher weight = more CPU share
  - EDF/RMS/LLF: Weight ignored

---

## Common Patterns

### Pattern 1: Sensor Reading (Fast, Critical)
```c
k_thread_rt_config(k_current_get(), 20, 5, 10);  // 50Hz, critical
```

### Pattern 2: Control Loop (Medium Speed)
```c
k_thread_rt_config(k_current_get(), 100, 20, 5);  // 10Hz, important
```

### Pattern 3: Logging (Slow, Background)
```c
k_thread_rt_config(k_current_get(), 1000, 50, 1);  // 1Hz, low priority
```

---

## Testing Your Scheduler

Use the built-in evaluation framework:

```bash
# Test your current scheduler
cd app/simple_eval_step1
west build -b native_sim
west build -t run > results/output.txt

# Test all schedulers automatically
bash scripts/run_all_tests.sh
python3 scripts/generate_graphs.py

# View graphs in results/graphs/
```

---

## Troubleshooting

### "Task not running"
Check all tasks have same priority (e.g., 5)  
Verify CONFIG_SCHED_DEADLINE=y in prj.conf  

### "Weights don't work"
Ensure using CONFIG_736_MOD_EDF, WSRT, or PFS  
Not plain CONFIG_SCHED_DEADLINE  

### "WSRT/LLF errors"
Add runtime tracking configs (see Step 1)  

### "Deadline misses"
Check utilization: sum(exec_time/period) should be < 1.0  
Reduce exec_time or increase period  

---

## Next Steps

- **See full examples**: Check `samples/scheduler_example/`
- **Understand algorithms**: Read `ALGORITHM_COMPARISON.md`
- **Deep dive**: See `SCHEDULER_USER_GUIDE.md`
- **Compare schedulers**: Run `app/simple_eval_step1/scripts/run_all_tests.sh`

---

## Cheat Sheet

| Config | Scheduler | Needs Runtime Stats? | Uses Weight? |
|--------|-----------|---------------------|--------------|
| `CONFIG_SCHED_DEADLINE` | EDF | No | No |
| `CONFIG_736_MOD_EDF` | Weighted EDF | No | Yes |
| `CONFIG_736_WSRT` | WSRT | Yes | Yes |
| `CONFIG_736_RMS` | RMS | No | No |
| `CONFIG_736_LLF` | LLF | Yes | No |
| `CONFIG_736_PFS` | PFS | No | Yes |

**Runtime stats = Add these to prj.conf:**
```kconfig
CONFIG_SCHED_THREAD_USAGE=y
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_736_TIME_LEFT=y
```

---

**You're all set! Happy scheduling!**
