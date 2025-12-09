#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include "workloads.h"
#include "metrics.h"

/* External references */
extern task_config_t task_configs[];
extern task_stats_t task_stats[];
extern bool tasks_running;

/**
 * Shell command: show - Display current configuration
 */
static int cmd_show(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    
    shell_print(sh, "\n═══════════════════════════════════════════════════");
    shell_print(sh, "Current Workload: %s", workload_name);
    shell_print(sh, "═══════════════════════════════════════════════════");
    
    float total_util = 0.0f;
    for (int i = 0; i < NUM_TASKS; i++) {
        const task_config_t *cfg = &task_configs[i];
        float util = cfg->exec_time_ms / (float)cfg->period_ms;
        total_util += util;
        
        shell_print(sh, "%s: P=%ums E=%ums D=%ums W=%u (%.1f%%)",
                   cfg->name, cfg->period_ms, cfg->exec_time_ms,
                   cfg->deadline_ms, cfg->weight, 100.0f * util);
    }
    
    shell_print(sh, "───────────────────────────────────────────────────");
    shell_print(sh, "Total Utilization: %.1f%%", 100.0f * total_util);
    shell_print(sh, "═══════════════════════════════════════════════════\n");
    
    return 0;
}

/**
 * Shell command: stats - Display runtime statistics
 */
static int cmd_stats(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    
    shell_print(sh, "\n═══════════════════════════════════════════════════");
    shell_print(sh, "Runtime Statistics");
    shell_print(sh, "═══════════════════════════════════════════════════");
    
    for (int i = 0; i < NUM_TASKS; i++) {
        const task_stats_t *stats = &task_stats[i];
        const task_config_t *cfg = &task_configs[i];
        
        if (stats->activations == 0) {
            shell_print(sh, "%s: No activations yet", cfg->name);
            continue;
        }
        
        uint32_t avg_rt = stats->total_response_time / stats->activations;
        double miss_rate = 100.0 * stats->deadline_misses / stats->activations;
        
        shell_print(sh, "%s: Act=%u Miss=%u (%.1f%%) AvgRT=%ums Jitter=%.2fms",
                   cfg->name, stats->activations, stats->deadline_misses,
                   miss_rate, avg_rt, stats->response_time_std_dev);
    }
    
    shell_print(sh, "═══════════════════════════════════════════════════\n");
    
    return 0;
}

/**
 * Shell command: format - Change output format
 */
static int cmd_format(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(sh, "Usage: format <csv|json|human|quiet>");
        shell_print(sh, "Current format: %s", 
                   output_format == OUTPUT_CSV ? "csv" :
                   output_format == OUTPUT_JSON ? "json" :
                   output_format == OUTPUT_HUMAN ? "human" : "quiet");
        return -1;
    }
    
    if (strcmp(argv[1], "csv") == 0) {
        output_format = OUTPUT_CSV;
        shell_print(sh, "Output format: CSV");
    } else if (strcmp(argv[1], "json") == 0) {
        output_format = OUTPUT_JSON;
        shell_print(sh, "Output format: JSON");
    } else if (strcmp(argv[1], "human") == 0) {
        output_format = OUTPUT_HUMAN;
        shell_print(sh, "Output format: Human-readable");
    } else if (strcmp(argv[1], "quiet") == 0) {
        output_format = OUTPUT_QUIET;
        shell_print(sh, "Output format: Quiet");
    } else {
        shell_error(sh, "Unknown format: %s", argv[1]);
        return -1;
    }
    
    return 0;
}

/**
 * Shell command: set - Modify task parameters at runtime
 */
static int cmd_set(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 4) {
        shell_print(sh, "Usage: set <task_id> <param> <value>");
        shell_print(sh, "  task_id: 1-%d", NUM_TASKS);
        shell_print(sh, "  param: period|exec|deadline|weight");
        shell_print(sh, "  value: integer value");
        return -1;
    }
    
    int task_id = atoi(argv[1]);
    if (task_id < 1 || task_id > NUM_TASKS) {
        shell_error(sh, "Invalid task_id: %d (must be 1-%d)", task_id, NUM_TASKS);
        return -1;
    }
    
    task_config_t *cfg = &task_configs[task_id - 1];
    int value = atoi(argv[3]);
    
    if (value <= 0) {
        shell_error(sh, "Invalid value: %d", value);
        return -1;
    }
    
    if (strcmp(argv[2], "period") == 0) {
        cfg->period_ms = value;
        shell_print(sh, "%s period set to %u ms", cfg->name, value);
    } else if (strcmp(argv[2], "exec") == 0) {
        cfg->exec_time_ms = value;
        shell_print(sh, "%s exec_time set to %u ms", cfg->name, value);
    } else if (strcmp(argv[2], "deadline") == 0) {
        cfg->deadline_ms = value;
        shell_print(sh, "%s deadline set to %u ms", cfg->name, value);
    } else if (strcmp(argv[2], "weight") == 0) {
        cfg->weight = value;
        shell_print(sh, "%s weight set to %u", cfg->name, value);
    } else {
        shell_error(sh, "Unknown parameter: %s", argv[2]);
        return -1;
    }
    
    return 0;
}

/**
 * Shell command: reset - Reset statistics
 */
static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    
    for (int i = 0; i < NUM_TASKS; i++) {
        task_stats[i].activations = 0;
        task_stats[i].deadline_misses = 0;
        task_stats[i].total_response_time = 0;
        task_stats[i].sum_response_time_squared = 0;
        task_stats[i].total_exec_time = 0;
        task_stats[i].total_lateness = 0;
    }
    
    metrics_init();
    shell_print(sh, "Statistics reset");
    
    return 0;
}

/**
 * Shell command: util - Calculate and display utilization
 */
static int cmd_util(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    
    float total_util = 0.0f;
    shell_print(sh, "\nTask Utilization:");
    
    for (int i = 0; i < NUM_TASKS; i++) {
        const task_config_t *cfg = &task_configs[i];
        float util = cfg->exec_time_ms / (float)cfg->period_ms;
        total_util += util;
        
        shell_print(sh, "  %s: %.2f%% (C=%u, T=%u)",
                   cfg->name, 100.0f * util, cfg->exec_time_ms, cfg->period_ms);
    }
    
    shell_print(sh, "Total: %.2f%%", 100.0f * total_util);
    
    if (total_util <= 1.0) {
        shell_print(sh, "Status: Schedulable");
    } else {
        shell_warn(sh, "Status: OVERLOADED!");
    }
    
    shell_print(sh, "");
    return 0;
}

/* Register shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(rt_eval_cmds,
    SHELL_CMD(show, NULL, "Show current configuration", cmd_show),
    SHELL_CMD(stats, NULL, "Display runtime statistics", cmd_stats),
    SHELL_CMD(format, NULL, "Set output format (csv|json|human|quiet)", cmd_format),
    SHELL_CMD(set, NULL, "Set task parameter (task_id param value)", cmd_set),
    SHELL_CMD(reset, NULL, "Reset statistics", cmd_reset),
    SHELL_CMD(util, NULL, "Show utilization analysis", cmd_util),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(rt, &rt_eval_cmds, "RT Scheduler Evaluation commands", NULL);
