/*
* Critical tasks implementations and utilities
*
* Additional critical task implementations and utilities
*/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(critical_tasks, LOG_LEVEL_DBG);

// External variables from main.c
extern struct k_mutex resource_mutex;
extern struct k_sem task_sync_sem;
extern volatile bool simulation_running;

// Additional critical task definitions
#define SAFETY_MONITOR_PRIORITY    2
#define FAULT_HANDLER_PRIORITY     1
#define EMERGENCY_TASK_PRIORITY    0  // Highest possible priority

#define CRITICAL_STACK_SIZE 1024

// Function prototypes
void emergency_response_task(void *arg1, void *arg2, void *arg3);
void safety_monitor_task(void *arg1, void *arg2, void *arg3);
void fault_detection_task(void *arg1, void *arg2, void *arg3);

// Emergency response task
K_THREAD_DEFINE(emergency_tid, CRITICAL_STACK_SIZE,
                emergency_response_task, NULL, NULL, NULL,
                EMERGENCY_TASK_PRIORITY, 0, 0);

// Safety monitor task
K_THREAD_DEFINE(safety_monitor_tid, CRITICAL_STACK_SIZE,
                safety_monitor_task, NULL, NULL, NULL,
                SAFETY_MONITOR_PRIORITY, 0, 0);

// Fault detection task
K_THREAD_DEFINE(fault_detector_tid, CRITICAL_STACK_SIZE,
                fault_detection_task, NULL, NULL, NULL,
                FAULT_HANDLER_PRIORITY, 0, 0);

// Critical system events
K_EVENT_DEFINE(system_events);

#define EVENT_EMERGENCY_STOP    BIT(0)
#define EVENT_SYSTEM_FAULT      BIT(1)
#define EVENT_SAFETY_VIOLATION  BIT(2)
#define EVENT_RESOURCE_CRITICAL BIT(3)

static uint32_t emergency_responses = 0;
static uint32_t safety_violations = 0;
static uint32_t fault_detections = 0;

void emergency_response_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t events;
    timing_t response_start, response_end;
    uint64_t response_time;

    LOG_INF("Emergency Response Task started - Priority: %d", EMERGENCY_TASK_PRIORITY);

    while (simulation_running) {
        // Wait for emergency events
        events = k_event_wait(&system_events, 
                              EVENT_EMERGENCY_STOP | EVENT_SYSTEM_FAULT,
                              false, K_MSEC(1000));
        
        if (events) {
            response_start = timing_counter_get();
            
            LOG_DBG("Emergency Response: Handling events: 0x%08X", events);
            
            if (events & EVENT_EMERGENCY_STOP) {
                LOG_ERR("EMERGENCY: STOP command received!");
                // Simulate emergency stop sequence
                k_busy_wait(1000); // 1ms critical response
            }
            
            if (events & EVENT_SYSTEM_FAULT) {
                LOG_ERR("EMERGENCY: System fault detected!");
                // Simulate fault recovery
                k_busy_wait(2000); // 2ms fault handling
            }
            
            response_end = timing_counter_get();
            response_time = timing_cycles_to_ns(response_end - response_start);
            
            LOG_INF("EMERGENCY: Response completed in %llu ns", response_time);
            emergency_responses++;
            
            // Clear handled events
            k_event_clear(&system_events, events);
        }
    }
    
    LOG_INF("Emergency Response Task handled %u emergencies", emergency_responses);
}

void safety_monitor_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t monitor_cycles = 0;
    timing_t monitor_start, monitor_end;
    uint64_t monitor_time;

    LOG_INF("Safety Monitor Task started - Priority: %d", SAFETY_MONITOR_PRIORITY);

    while (simulation_running) {
        monitor_start = timing_counter_get();
        
        LOG_DBG("Safety Monitor: Cycle %u - Checking system parameters", monitor_cycles);
        
        // Simulate safety parameter monitoring
        k_busy_wait(1500); // 1.5ms monitoring overhead
        
        // Periodically trigger safety events for testing
        if ((monitor_cycles % 100) == 0 && monitor_cycles > 0) {
            LOG_WRN("Safety Monitor: Safety violation detected!");
            k_event_post(&system_events, EVENT_SAFETY_VIOLATION);
            safety_violations++;
        }
        
        // Trigger emergency events occasionally
        if ((monitor_cycles % 300) == 0 && monitor_cycles > 0) {
            LOG_ERR("Safety Monitor: Critical fault - triggering emergency!");
            k_event_post(&system_events, EVENT_SYSTEM_FAULT);
        }
        
        monitor_end = timing_counter_get();
        monitor_time = timing_cycles_to_ns(monitor_end - monitor_start);
        
        if (monitor_time > 5000000) { // 5ms threshold
            LOG_WRN("Safety Monitor: Monitoring cycle took %llu ns (>5ms)", monitor_time);
        }
        
        LOG_INF("SAFETY: Monitor cycle %u completed - Thread %p", 
                monitor_cycles, k_current_get());
        
        monitor_cycles++;
        k_msleep(25); // 40Hz monitoring rate
    }
    
    LOG_INF("Safety Monitor Task completed %u cycles, detected %u violations", 
            monitor_cycles, safety_violations);
}

void fault_detection_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t detection_cycles = 0;
    uint32_t false_positives = 0;

    LOG_INF("Fault Detection Task started - Priority: %d", FAULT_HANDLER_PRIORITY);

    while (simulation_running) {
        LOG_DBG("Fault Detection: Scanning cycle %u", detection_cycles);
        
        // Simulate fault detection algorithms
        k_busy_wait(4000); // 4ms detection processing
        
        // Simulate occasional fault detection
        if ((detection_cycles % 150) == 0 && detection_cycles > 0) {
            // Acquire resource for fault analysis
            if (k_mutex_lock(&resource_mutex, K_MSEC(5)) == 0) {
                LOG_WRN("Fault Detector: Potential fault analyzed");
                
                // Simulate fault analysis
                k_busy_wait(3000); // 3ms analysis
                
                // Determine if it's a real fault or false positive
                if (sys_rand32_get() % 4 == 0) {
                    LOG_ERR("Fault Detector: Real fault confirmed!");
                    k_event_post(&system_events, EVENT_SYSTEM_FAULT);
                    fault_detections++;
                } else {
                    LOG_DBG("Fault Detector: False positive detected");
                    false_positives++;
                }
                
                k_mutex_unlock(&resource_mutex);
            } else {
                LOG_WRN("Fault Detector: Could not acquire mutex for analysis");
            }
        }
        
        LOG_INF("FAULT: Detection cycle %u - Thread %p", 
                detection_cycles, k_current_get());
        
        detection_cycles++;
        k_msleep(40); // 25Hz detection rate
    }
    
    LOG_INF("Fault Detection Task completed %u cycles, found %u faults, %u false positives", 
            detection_cycles, fault_detections, false_positives);
}

void init_critical_tasks(void)
{
    LOG_INF("Initializing critical tasks and event system...");

    // Initialize system events
    k_event_init(&system_events);

    // Set thread names for better debugging
    k_thread_name_set(emergency_tid, "emergency_response");
    k_thread_name_set(safety_monitor_tid, "safety_monitor");
    k_thread_name_set(fault_detector_tid, "fault_detector");

    LOG_INF("Critical tasks initialized");
    LOG_INF("Emergency Response Priority: %d", EMERGENCY_TASK_PRIORITY);
    LOG_INF("Safety Monitor Priority: %d", SAFETY_MONITOR_PRIORITY);
    LOG_INF("Fault Detection Priority: %d", FAULT_HANDLER_PRIORITY);
}