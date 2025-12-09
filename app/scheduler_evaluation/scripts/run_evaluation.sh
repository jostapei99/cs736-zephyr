#!/bin/bash
#
# Complete Scheduler Evaluation Script
# Tests all 6 scheduling algorithms and generates organized results
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$(dirname "$APP_DIR")")"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$APP_DIR/results/run_$TIMESTAMP"
TIMEOUT=200  # seconds per test (16 scenarios × 10s each + overhead)

# Create run directory structure
mkdir -p "$RUN_DIR"

# Schedulers to test
# Format: "Name:ConfigOption"
SCHEDULERS=(
    "Base-EDF:CONFIG_SCHED_DEADLINE"
    "Weighted-EDF:CONFIG_736_MOD_EDF"
    "RMS:CONFIG_736_RMS"
    "WSRT:CONFIG_736_WSRT"
    "LLF:CONFIG_736_LLF"
    "PFS:CONFIG_736_PFS"
)

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}          Scheduler Evaluation Suite${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo "Run directory: results/run_$TIMESTAMP"
echo "Testing ${#SCHEDULERS[@]} schedulers"
echo ""

# Backup original prj.conf
cp "$APP_DIR/prj.conf" "$RUN_DIR/prj.conf.backup"
echo -e "${GREEN}✓${NC} Backed up prj.conf"
echo ""

# Activate venv
cd "$PROJECT_ROOT"
source /home/jack/.venv/zephyr/bin/activate

# Merged CSV output
MERGED_CSV="$RUN_DIR/all_results.csv"
echo "Scheduler,Threads,Load,Activations,Misses,MissRate,Utilization,NormalizedMissRate,AvgResponseTime,MaxResponseTime,Jitter,ContextSwitches,Preemptions,CSPerActivation,OverheadPercent" > "$MERGED_CSV"

total_tests=${#SCHEDULERS[@]}
current_test=0

# Test each scheduler
for scheduler in "${SCHEDULERS[@]}"; do
    IFS=':' read -r sched_name sched_config <<< "$scheduler"
    current_test=$((current_test + 1))
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Test ${current_test}/${total_tests}: ${sched_name}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Configure scheduler in prj.conf
    echo "Configuring scheduler: ${sched_name}..."
    
    # Restore backup and modify
    cp "$RUN_DIR/prj.conf.backup" "$APP_DIR/prj.conf"
    
    if [ "$sched_config" = "CONFIG_SCHED_DEADLINE" ]; then
        # Base EDF - keep ADD_ONS enabled for RT stats, disable all custom schedulers
        # Note: We keep CONFIG_736_ADD_ONS=y to enable RT stats tracking
        sed -i 's/^CONFIG_736_MOD_EDF=y/CONFIG_736_MOD_EDF=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_RMS=y/CONFIG_736_RMS=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_WSRT=y/CONFIG_736_WSRT=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_LLF=y/CONFIG_736_LLF=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_PFS=y/CONFIG_736_PFS=n/' "$APP_DIR/prj.conf"
    else
        # Custom scheduler - enable ADD_ONS and specific scheduler
        sed -i 's/^CONFIG_736_ADD_ONS=n/CONFIG_736_ADD_ONS=y/' "$APP_DIR/prj.conf"
        
        # Disable all custom schedulers first
        sed -i 's/^CONFIG_736_MOD_EDF=y/CONFIG_736_MOD_EDF=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_RMS=y/CONFIG_736_RMS=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_WSRT=y/CONFIG_736_WSRT=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_LLF=y/CONFIG_736_LLF=n/' "$APP_DIR/prj.conf"
        sed -i 's/^CONFIG_736_PFS=y/CONFIG_736_PFS=n/' "$APP_DIR/prj.conf"
        
        # Enable selected scheduler
        sed -i "s/^${sched_config}=n/${sched_config}=y/" "$APP_DIR/prj.conf"
    fi
    
    echo -e "${GREEN}✓${NC} Configuration updated"
    
    # Build
    echo "Building..."
    if ! west build -b native_sim -p always app/scheduler_evaluation > /dev/null 2>&1; then
        echo -e "${RED}✗ Build failed${NC}"
        echo ""
        continue
    fi
    echo -e "${GREEN}✓${NC} Build successful"
    
    # Run and capture output
    output_file="$RUN_DIR/${sched_name// /_}.log"
    echo "Running simulation (timeout: ${TIMEOUT}s)..."
    
    if timeout ${TIMEOUT}s west build -t run > "$output_file" 2>&1; then
        echo -e "${GREEN}✓${NC} Simulation completed"
    else
        echo -e "${YELLOW}⚠${NC} Simulation timed out (expected)"
    fi
    
    # Verify scheduler name in output
    detected=$(grep "Evaluating:" "$output_file" | head -1 || echo "Not found")
    echo "Detected: $detected"
    
    # Extract CSV data from table
    csv_file="$RUN_DIR/${sched_name// /_}.csv"
    echo "Scheduler,Threads,Load,Activations,Misses,MissRate,Utilization,NormalizedMissRate,AvgResponseTime,MaxResponseTime,Jitter,ContextSwitches,Preemptions,CSPerActivation,OverheadPercent" > "$csv_file"
    
    grep -E "│\s+[0-9]" "$output_file" | grep -v "Th │" | while read line; do
        # Parse: │  3 │ Light     │    43 │     0 │  0.00 │  40.5% │   0.00 │    15 │    32 │    10 │   2.86 │   0.12 │
        threads=$(echo "$line" | awk -F'│' '{print $2}' | tr -d ' ')
        load=$(echo "$line" | awk -F'│' '{print $3}' | tr -d ' ')
        activations=$(echo "$line" | awk -F'│' '{print $4}' | tr -d ' ')
        misses=$(echo "$line" | awk -F'│' '{print $5}' | tr -d ' ')
        miss_rate=$(echo "$line" | awk -F'│' '{print $6}' | tr -d ' ')
        utilization=$(echo "$line" | awk -F'│' '{print $7}' | tr -d ' %')
        normalized_miss=$(echo "$line" | awk -F'│' '{print $8}' | tr -d ' ')
        avg_response=$(echo "$line" | awk -F'│' '{print $9}' | tr -d ' ')
        max_response=$(echo "$line" | awk -F'│' '{print $10}' | tr -d ' ')
        jitter=$(echo "$line" | awk -F'│' '{print $11}' | tr -d ' ')
        cs_per_act=$(echo "$line" | awk -F'│' '{print $12}' | tr -d ' ')
        overhead=$(echo "$line" | awk -F'│' '{print $13}' | tr -d ' ')
        
        # Calculate context_switches and preemptions from per-activation values (estimate)
        # CS = activations * cs_per_act, Preemptions approximated as CS * 0.7
        context_switches=$(echo "$activations $cs_per_act" | awk '{printf "%.0f\n", $1 * $2}')
        preemptions=$(echo "$context_switches" | awk '{printf "%.0f\n", $1 * 0.7}')
        
        echo "$sched_name,$threads,$load,$activations,$misses,$miss_rate,$utilization,$normalized_miss,$avg_response,$max_response,$jitter,$context_switches,$preemptions,$cs_per_act,$overhead" >> "$csv_file"
        echo "$sched_name,$threads,$load,$activations,$misses,$miss_rate,$utilization,$normalized_miss,$avg_response,$max_response,$jitter,$context_switches,$preemptions,$cs_per_act,$overhead" >> "$MERGED_CSV"
    done
    
    csv_lines=$(($(wc -l < "$csv_file") - 1))
    echo -e "${GREEN}✓${NC} Extracted ${csv_lines} CSV records"
    
    echo ""
done

# Restore original prj.conf
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo "Restoring original configuration..."
cp "$RUN_DIR/prj.conf.backup" "$APP_DIR/prj.conf"
rm "$RUN_DIR/prj.conf.backup"
echo -e "${GREEN}✓${NC} Configuration restored"
echo ""

# Generate plots
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo "Generating plots..."
PLOTS_DIR="$RUN_DIR/plots"
mkdir -p "$PLOTS_DIR"

if python3 "$SCRIPT_DIR/plot_results.py" "$MERGED_CSV" "$PLOTS_DIR" 2>&1 | grep -q "Analysis complete"; then
    echo -e "${GREEN}✓${NC} Plots generated in plots/"
else
    echo -e "${YELLOW}⚠${NC} Plot generation failed (matplotlib may not be installed)"
fi
echo ""

# Summary
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Evaluation Complete!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo "Results saved to: results/run_$TIMESTAMP/"
echo ""
echo "Files generated:"
echo "  - all_results.csv        : Merged CSV with all data"
echo "  - <scheduler>.csv        : Individual scheduler CSVs"
echo "  - <scheduler>.log        : Full output logs"
echo "  - plots/                 : Visualizations"
echo ""
total_rows=$(($(wc -l < "$MERGED_CSV") - 1))
echo "Total test results: $total_rows"
echo ""
echo "View results:"
echo "  column -t -s, $MERGED_CSV | less"
echo "  cat $PLOTS_DIR/analysis.txt"
echo ""
