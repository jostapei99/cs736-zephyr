#!/bin/bash
# Complete evaluation workflow for advanced_eval
# Runs tests and generates all graphs automatically

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="$APP_DIR/results"

echo "════════════════════════════════════════════════════════════════════════════════"
echo "           Advanced RT Scheduler Evaluation - Complete Workflow"
echo "════════════════════════════════════════════════════════════════════════════════"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

# Check if Python dependencies are installed
check_dependencies() {
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo "Step 1: Checking Python Dependencies"
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo ""
    
    if ! python3 "$SCRIPT_DIR/check_deps.py"; then
        print_error "Missing Python dependencies"
        echo ""
        echo "Install with: pip3 install pandas matplotlib seaborn numpy"
        exit 1
    fi
    
    print_status "All Python dependencies installed"
    echo ""
}

# Run test suite
run_tests() {
    local test_type=$1
    
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo "Step 2: Running Test Suite ($test_type)"
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo ""
    
    if [ "$test_type" = "quick" ]; then
        print_info "Running quick demo (4 tests, ~80 seconds)"
        echo ""
        bash "$SCRIPT_DIR/quick_demo.sh"
    else
        print_info "Running full test suite (16 tests, ~10 minutes)"
        echo ""
        bash "$SCRIPT_DIR/run_all_tests.sh"
    fi
    
    if [ $? -eq 0 ]; then
        print_status "Test suite completed successfully"
    else
        print_error "Test suite failed"
        exit 1
    fi
    echo ""
}

# Generate graphs
generate_graphs() {
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo "Step 3: Generating Graphs"
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo ""
    
    print_info "Creating visualizations from CSV data..."
    echo ""
    
    python3 "$SCRIPT_DIR/generate_graphs.py"
    
    if [ $? -eq 0 ]; then
        print_status "Graphs generated successfully"
    else
        print_error "Graph generation failed"
        exit 1
    fi
    echo ""
}

# Display results summary
show_summary() {
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo "Step 4: Results Summary"
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo ""
    
    if [ -f "$RESULTS_DIR/graphs/summary_report.txt" ]; then
        print_status "Summary report generated"
        echo ""
        echo "Quick preview:"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        head -n 30 "$RESULTS_DIR/graphs/summary_report.txt"
        echo ""
        echo "... (see full report in results/graphs/summary_report.txt)"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    fi
    
    echo ""
    print_info "Results location:"
    echo "  CSV files:       $RESULTS_DIR/*.csv"
    echo "  Summary files:   $RESULTS_DIR/*_summary.txt"
    echo "  Graphs:          $RESULTS_DIR/graphs/*.png"
    echo "  Summary report:  $RESULTS_DIR/graphs/summary_report.txt"
    echo ""
    
    # Count files
    csv_count=$(find "$RESULTS_DIR" -maxdepth 1 -name "*.csv" 2>/dev/null | wc -l)
    summary_count=$(find "$RESULTS_DIR" -maxdepth 1 -name "*_summary.txt" 2>/dev/null | wc -l)
    graph_count=$(find "$RESULTS_DIR/graphs" -name "*.png" 2>/dev/null | wc -l)
    
    print_status "Generated $csv_count CSV files"
    print_status "Generated $summary_count summary files"
    print_status "Generated $graph_count graph images"
    echo ""
}

# Interactive menu
show_menu() {
    echo "Select evaluation mode:"
    echo ""
    echo "  1) Quick Demo    - Test 4 scheduler/workload combinations (~80 seconds)"
    echo "  2) Full Suite    - Test all 16 combinations (~10 minutes)"
    echo "  3) Graphs Only   - Generate graphs from existing CSV data"
    echo "  4) Exit"
    echo ""
    echo -n "Enter choice [1-4]: "
}

# Main execution
main() {
    cd "$APP_DIR"
    
    # Check dependencies first
    check_dependencies
    
    # If no arguments, show interactive menu
    if [ $# -eq 0 ]; then
        while true; do
            show_menu
            read -r choice
            echo ""
            
            case $choice in
                1)
                    run_tests "quick"
                    generate_graphs
                    show_summary
                    break
                    ;;
                2)
                    run_tests "full"
                    generate_graphs
                    show_summary
                    break
                    ;;
                3)
                    if [ ! -d "$RESULTS_DIR" ] || [ $(find "$RESULTS_DIR" -maxdepth 1 -name "*.csv" | wc -l) -eq 0 ]; then
                        print_error "No CSV data found!"
                        echo "  Run tests first (option 1 or 2)"
                        echo ""
                        continue
                    fi
                    generate_graphs
                    show_summary
                    break
                    ;;
                4)
                    echo "Exiting."
                    exit 0
                    ;;
                *)
                    print_error "Invalid choice. Please enter 1-4."
                    echo ""
                    ;;
            esac
        done
    else
        # Command line argument provided
        case "$1" in
            quick|--quick|-q)
                run_tests "quick"
                generate_graphs
                show_summary
                ;;
            full|--full|-f)
                run_tests "full"
                generate_graphs
                show_summary
                ;;
            graphs|--graphs|-g)
                if [ ! -d "$RESULTS_DIR" ] || [ $(find "$RESULTS_DIR" -maxdepth 1 -name "*.csv" | wc -l) -eq 0 ]; then
                    print_error "No CSV data found! Run tests first."
                    exit 1
                fi
                generate_graphs
                show_summary
                ;;
            help|--help|-h)
                echo "Usage: $0 [option]"
                echo ""
                echo "Options:"
                echo "  quick, -q     Run quick demo (4 tests)"
                echo "  full, -f      Run full test suite (16 tests)"
                echo "  graphs, -g    Generate graphs only (requires existing CSV data)"
                echo "  help, -h      Show this help message"
                echo ""
                echo "If no option is provided, an interactive menu will be shown."
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    fi
    
    echo "════════════════════════════════════════════════════════════════════════════════"
    echo "                           ✓ Evaluation Complete!"
    echo "════════════════════════════════════════════════════════════════════════════════"
    echo ""
    print_status "All tasks completed successfully"
    echo ""
    print_info "Next steps:"
    echo "  • Review graphs in: $RESULTS_DIR/graphs/"
    echo "  • Read summary report: $RESULTS_DIR/graphs/summary_report.txt"
    echo "  • Analyze CSV data: $RESULTS_DIR/*.csv"
    echo "  • Check individual summaries: $RESULTS_DIR/*_summary.txt"
    echo ""
}

# Run main function
main "$@"
