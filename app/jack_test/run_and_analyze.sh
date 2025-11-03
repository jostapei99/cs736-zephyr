#!/bin/bash
# Zephyr RTOS Test Runner and Output Capture Script

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Cleanup function to kill any remaining QEMU processes
cleanup() {
    echo "Cleaning up..."
    pkill -f "qemu-system-i386.*zephyr.elf" 2>/dev/null || true
    rm -f "$SCRIPT_DIR/build/qemu.pid"
}

# Set up cleanup on script exit
trap cleanup EXIT
OUTPUT_DIR="${SCRIPT_DIR}/results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="${OUTPUT_DIR}/zephyr_run_${TIMESTAMP}.log"

# Create results directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo "Starting Zephyr RTOS Simulation..."
echo "Output will be saved to: $LOG_FILE"

# Ensure we're in the right directory
cd "$SCRIPT_DIR" || exit 1

# Clean up any existing QEMU processes and PID files
echo "Cleaning up any existing QEMU instances..."
pkill -f "qemu-system-i386.*zephyr.elf" 2>/dev/null || true
rm -f build/qemu.pid
sleep 1

# Build the application first
echo "Building Zephyr application..."
west build > "${OUTPUT_DIR}/build_${TIMESTAMP}.log" 2>&1

if [ $? -eq 0 ]; then
    echo "Build successful, starting simulation..."
    
    # Run with timeout and capture output  
    echo "  Running simulation for 40 seconds (30s simulation + 10s buffer)..."
    echo "  Capturing output to: $LOG_FILE"
    
    # Try running QEMU in a way that doesn't get interrupted
    echo "  Starting QEMU simulation..."
    
    # Use a different approach - run in background and wait
    west build -t run > "$LOG_FILE" 2>&1 &
    QEMU_PID=$!
    echo "  QEMU started with PID: $QEMU_PID"
    
    # Wait for 35 seconds or until QEMU finishes
    for i in {1..35}; do
        if ! kill -0 $QEMU_PID 2>/dev/null; then
            echo "QEMU finished naturally at ${i}s"
            break
        fi
        sleep 1
        if [ $i -eq 35 ]; then
            echo "Timeout reached, terminating QEMU..."
            kill -TERM $QEMU_PID 2>/dev/null || true
            sleep 2
            kill -KILL $QEMU_PID 2>/dev/null || true
        fi
    done
    
    wait $QEMU_PID 2>/dev/null
    EXIT_CODE=$?
    
    # Clean up PID file
    rm -f build/qemu.pid
    
    echo "  Simulation finished with exit code: $EXIT_CODE"
    echo "  Captured $(wc -l < "$LOG_FILE") lines of output"
    
    # Check if we got actual simulation data
    if grep -q "Simulation Time Elapsed" "$LOG_FILE"; then
        echo "  Simulation completed successfully"
        
        echo -e "\n Parsing simulation output..."
        
        # Parse the output with summary
        echo "=== SIMULATION SUMMARY ==="
        python3 "${SCRIPT_DIR}/parse_output.py" "$LOG_FILE" --summary
        
        # Export JSON data
        JSON_FILE="${OUTPUT_DIR}/zephyr_data_${TIMESTAMP}.json"
        echo -e "\nExporting data to JSON..."
        python3 "${SCRIPT_DIR}/parse_output.py" "$LOG_FILE" --output "$JSON_FILE"
        
        # Generate performance report if analyzer exists
        if [ -f "${SCRIPT_DIR}/performance_analyzer.py" ]; then
            echo -e "\nGenerating detailed performance analysis..."
            python3 "${SCRIPT_DIR}/performance_analyzer.py" "$LOG_FILE"
        fi
        
        # Show example analysis
        if [ -f "${SCRIPT_DIR}/example_analysis.py" ]; then
            echo -e "\nExample Analysis Results:"
            python3 "${SCRIPT_DIR}/example_analysis.py" "$JSON_FILE"
        fi
        
        echo -e "\nAnalysis complete! Results saved to:"
        echo "   Log file: $LOG_FILE"
        echo "   JSON data: $JSON_FILE"
    else
        echo "ERROR: Simulation did not complete properly or no timing data found"
        echo "Log file size: $(wc -l < "$LOG_FILE") lines"
        echo "Recent log content:"
        tail -10 "$LOG_FILE"
        exit 1
    fi
else
    echo "ERROR: Build failed"
    exit 1
fi