/*
    Mission Critical Scheduler Simulation

    This applicaion simulates mission-critical real-time tasks with various
    priorities and deadlines to analyze scheduler behavior.
*/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel_version.h>
#include <zephyr/random/random.h>
#include <zephyr/kernel_version.h>
#include "mission_critical.h"

LOG_MODULE_REGISTER(mission_critical, LOG_LEVEL_DBG);


// external function declarations
extern void init_critical_tasks(void);
extern void start_timing_analysis(void);
extern void log_scheduler_state(void);

// Mission control task - highest priority
K_THREAD_DEFINE(mission_control_tid, TASK_STACK_SIZE,
                mission_control_task, NULL, NULL, NULL,
                CRITICAL_TASK_PRIORITY, 0, 0);

// Navigation task - high priority
K_THREAD_DEFINE(navigation_tid, TASK_STACK_SIZE,
                navigation_task, NULL, NULL, NULL,
                HIGH_PRIORITY_TASK, 0, 0);

// Communication task - medium priority
K_THREAD_DEFINE(communication_tid, TASK_STACK_SIZE,
                communication_task, NULL, NULL, NULL,
                MEDIUM_PRIORITY_TASK, 0, 0);

// Housekeeping task - low priority
K_THREAD_DEFINE(housekeeping_tid, TASK_STACK_SIZE,
                housekeeping_task, NULL, NULL, NULL,
                LOW_PRIORITY_TASK, 0, 0);

// Shared resources and synchronization
K_MUTEX_DEFINE(resource_mutex);
K_SEM_DEFINE(task_sync_sem, 0, 4);

static uint32_t simulation_start_time;
volatile bool simulation_running = true;

void mission_control_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t cycle_count = 0;
    timing_t start_time, end_time;
    uint64_t execution_time;

    LOG_INF("Mission Control Task started - Priority: %d", CRITICAL_TASK_PRIORITY);

    while (simulation_running) {
        timing_start();
        start_time = timing_counter_get();
        
        LOG_DBG("Mission Control: Cycle %u - Critical safety check", cycle_count);

        // simulate critical mission control operations
        k_busy_wait(5000); // 5ms of critical processing

        // Log scheduler context
        LOG_INF("MC: Thread %p executing at cycle %u", k_current_get(), cycle_count);

        // check for deadline adherence
        end_time = timing_counter_get();
        execution_time = timing_cycles_to_ns(end_time - start_time);
        
        if (execution_time > (DEADLINE_MS * 1000000 / 2)) {
            LOG_WRN("Mission Control: Potential deadline miss! Execution: %llu ns", 
                    execution_time);
        }
        
        timing_stop();
        
        cycle_count++;
        k_msleep(50); // 20Hz execution rate
    }
    
    LOG_INF("Mission Control Task completed %u cycles", cycle_count);
}

void navigation_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t nav_updates = 0;
    
    LOG_INF("Navigation Task started - Priority: %d", HIGH_PRIORITY_TASK);

    while (simulation_running) {
        LOG_DBG("Navigation: Updating position estimate %u", nav_updates);
        
        // Simulate navigation processing
        if (k_mutex_lock(&resource_mutex, K_MSEC(100)) == 0) {
            k_busy_wait(4000); // 4ms of navigation calculations
            
            LOG_INF("NAV: Position updated - Thread %p", k_current_get());
            
            k_mutex_unlock(&resource_mutex);
        } else {
            LOG_WRN("Navigation: Failed to acquire resource mutex!");
        }
        
        nav_updates++;
        k_msleep(75); // ~13Hz navigation updates
    }
    
    LOG_INF("Navigation Task completed %u updates", nav_updates);
}

void communication_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t messages_sent = 0;
    
    LOG_INF("Communication Task started - Priority: %d", MEDIUM_PRIORITY_TASK);

    while (simulation_running) {
        LOG_DBG("Communication: Sending telemetry message %u", messages_sent);

        // Simulate communication overhead
        k_busy_wait(3000 + sys_rand32_get() % 5000); // 3-8ms variable processing
        
        LOG_INF("COMM: Message %u transmitted - Thread %p", 
                messages_sent, k_current_get());
        
        messages_sent++;
        
        // Signal task synchronization
        k_sem_give(&task_sync_sem);
        
        k_msleep(200); // 5Hz communication rate
    }
    
    LOG_INF("Communication Task sent %u messages", messages_sent);
}

void housekeeping_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t housekeeping_cycles = 0;
    
    LOG_INF("Housekeeping Task started - Priority: %d", LOW_PRIORITY_TASK);

    while (simulation_running) {
        LOG_DBG("Housekeeping: Performing maintenance cycle %u", housekeeping_cycles);
        
        // simulate various housekeeping operations
        k_busy_wait(2000); // 2ms of housekeeping
        
        LOG_INF("HOUSE: Maintenance completed - Thread %p", k_current_get());
        
        housekeeping_cycles++;
        k_msleep(1000); // 1Hz housekeeping rate
    }
    
    LOG_INF("Housekeeping Task completed %u cycles", housekeeping_cycles);
}

void log_scheduler_state(void)
{
    struct k_thread *current = k_current_get();
    int raw_prio = current->base.prio;
    const char* thread_type;
    
    // Interpret Zephyr's internal priority representation
    if (raw_prio < 0) {
        thread_type = "COOPERATIVE";
    } else if (raw_prio <= 15) {
        thread_type = "PREEMPTIVE";
    } else {
        thread_type = "SYSTEM/IDLE";
    }
    
    LOG_INF("Scheduler State: Current Thread %p, Priority %d (%s)",
            current, raw_prio, thread_type);
}

static void simulation_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    uint32_t elapsed = k_uptime_get_32() - simulation_start_time;

    LOG_INF("Simulation Time Elapsed: %u seconds", elapsed / 1000);
    log_scheduler_state();

    if (elapsed >= (SIMULATION_DURATION_SEC * 1000)) {
        simulation_running = false;
        LOG_INF("Simulation duration reached. Stopping all tasks.");
    }
}

K_TIMER_DEFINE(simulation_timer, simulation_timer_handler, NULL);

int main(void)
{
    LOG_INF("=== Mission Critical Scheduler Simulation Starting ===");
    LOG_INF("Zephyr kernel version: %u", sys_kernel_version_get());
    
    simulation_start_time = k_uptime_get_32();
    
    /* Initialize timing subsystem */
    timing_init();
    LOG_INF("Timing subsystem initialized");
    
    /* Initialize critical tasks */
    init_critical_tasks();
    
    /* Start periodic simulation monitoring */
    k_timer_start(&simulation_timer, K_SECONDS(1), K_SECONDS(1));
    
    LOG_INF("All tasks initialized. Starting simulation for %d seconds...", 
            SIMULATION_DURATION_SEC);
    
    /* Start timing analysis */
    start_timing_analysis();
    
    /* Wait for simulation completion */
    while (simulation_running) {
        k_msleep(100);
    }
    
    /* Stop the simulation timer */
    k_timer_stop(&simulation_timer);
    
    LOG_INF("=== Mission Critical Scheduler Simulation Completed ===");
    
    return 0;
}