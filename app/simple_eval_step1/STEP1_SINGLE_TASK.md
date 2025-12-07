# Step 1: Single Periodic Task

## Objective
Create your first real-time periodic task that executes every 50ms and measures its own execution time.

## What You'll Build
A simple Zephyr application with:
- 1 periodic task
- Time measurement using cycle counters
- Console output to verify timing

## Template Structure

Create these files:
```
workloads_v2/step1/
├── CMakeLists.txt          # Build configuration
├── prj.conf                # Project configuration
├── src/
│   └── main.c              # Your implementation
```

## File: CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(step1_single_task)

target_sources(app PRIVATE src/main.c)
```

## File: prj.conf
```
# Enable console output
CONFIG_PRINTK=y
CONFIG_CONSOLE=y

# Increase main stack size for safety
CONFIG_MAIN_STACK_SIZE=2048
```

## File: src/main.c - YOUR TASK

Fill in the TODOs below:

```c
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* Task configuration */
#define TASK_PERIOD_MS      50      /* Task runs every 50ms */
#define TASK_WORK_US        10000   /* Simulated work: 10ms */
#define STACK_SIZE          1024
#define TASK_PRIORITY       7

/* TODO 1: Declare thread stack using K_THREAD_STACK_DEFINE */
/* Hint: K_THREAD_STACK_DEFINE(name, size) */


/* TODO 2: Declare thread structure using struct k_thread */


/**
 * Simulate computational work by busy-waiting
 * This represents actual CPU-intensive work a real task would do
 */
static void busy_wait_microseconds(uint32_t microseconds)
{
	uint32_t start = k_cycle_get_32();
	uint32_t cycles_to_wait = k_us_to_cyc_ceil32(microseconds);
	
	while ((k_cycle_get_32() - start) < cycles_to_wait) {
		/* Busy wait - CPU is actively working */
	}
}

/**
 * The periodic task function
 * This is where your task logic goes
 */
void task_function(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	
	uint32_t activation_count = 0;
	
	printk("Task started! Period=%dms, Work=%dus\n", 
	       TASK_PERIOD_MS, TASK_WORK_US);
	
	while (1) {
		/* TODO 3: Get the start time using k_cycle_get_32() */
		uint32_t start_time = /* YOUR CODE HERE */;
		
		/* Simulate computational work */
		busy_wait_microseconds(TASK_WORK_US);
		
		/* TODO 4: Get the end time using k_cycle_get_32() */
		uint32_t end_time = /* YOUR CODE HERE */;
		
		/* TODO 5: Calculate execution time in cycles */
		uint32_t execution_cycles = /* YOUR CODE HERE */;
		
		/* TODO 6: Convert cycles to microseconds using k_cyc_to_us_floor32() */
		uint32_t execution_us = /* YOUR CODE HERE */;
		
		activation_count++;
		
		/* Print results every activation */
		printk("Activation %u: Execution time = %u us (%u cycles)\n",
		       activation_count, execution_us, execution_cycles);
		
		/* TODO 7: Sleep until next period using k_msleep() */
		/* YOUR CODE HERE */
	}
}

int main(void)
{
	printk("\n=== Step 1: Single Periodic Task ===\n\n");
	
	/* TODO 8: Create the thread using k_thread_create() */
	/*
	 * Hint: k_thread_create(
	 *          &thread_struct,
	 *          stack,
	 *          K_THREAD_STACK_SIZEOF(stack),
	 *          task_function,
	 *          arg1, arg2, arg3,
	 *          priority,
	 *          options,
	 *          delay
	 *       );
	 */
	
	/* YOUR CODE HERE */
	
	printk("Thread created successfully\n");
	
	/* Main thread can sleep - the task thread will run */
	return 0;
}
```

## Build and Run

```bash
# Navigate to step1 directory
cd /home/jack/cs736-project/zephyr/app/workloads_v2/step1

# Build for native_sim target
west build -b native_sim

# Run the application
west build -t run
```

## Expected Output

You should see something like:
```
=== Step 1: Single Periodic Task ===

Task started! Period=50ms, Work=10000us
Thread created successfully
Activation 1: Execution time = 10001 us (320032 cycles)
Activation 2: Execution time = 10000 us (320000 cycles)
Activation 3: Execution time = 10001 us (320032 cycles)
...
```

## Verification Questions

Once it's running, answer these:

1. **Is the execution time close to 10ms (10000us)?**
   - If yes, your busy_wait is working correctly
   - If no, why might it differ?

2. **How accurate is the 50ms period?**
   - Modify the code to also print the time between activations
   - Is it exactly 50ms or does it vary?

3. **What happens if you increase TASK_WORK_US to 60000 (60ms)?**
   - This exceeds the period - what behavior do you observe?
   - Does the task still run? How often?

4. **CPU usage consideration:**
   - During busy_wait, is the CPU idle or active?
   - During k_msleep, is the CPU idle or active?
   - Why is this distinction important?

## Next Steps

Once you have this working and understand:
- How to create threads
- How timing works in Zephyr
- The difference between busy-wait and sleep

You're ready for **Step 2: Multiple Tasks**!

## Common Issues

**Issue**: Thread doesn't start
- Check that you called k_thread_create() correctly
- Verify stack and thread struct are declared

**Issue**: Execution time is 0 or wrong
- Make sure you're calculating end_time - start_time
- Check cycle counter overflow (unlikely in short runs)

**Issue**: Output stops appearing
- Your main() returned - threads were killed
- Keep main alive with `while(1) k_sleep(K_FOREVER);`
