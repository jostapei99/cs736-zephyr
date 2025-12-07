#!/bin/bash
#
# Master Script - Complete Evaluation Workflow
# Runs all tests and generates all graphs in one command
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  RT Scheduler Evaluation - Complete Workflow              ║${NC}"
echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo ""

# Step 1: Check dependencies
echo -e "${YELLOW}Step 1: Checking Python dependencies...${NC}"
if ! python3 "${SCRIPT_DIR}/check_deps.py"; then
    echo ""
    echo -e "${YELLOW}Please install missing dependencies first!${NC}"
    exit 1
fi
echo ""

# Step 2: Ask user for test type
echo -e "${YELLOW}Step 2: Select test mode${NC}"
echo "  1) Quick Demo (4 tests, ~1 minute)"
echo "  2) Full Test Suite (16 tests, ~10 minutes)"
echo ""
read -p "Enter choice [1-2]: " choice

case $choice in
    1)
        echo ""
        echo -e "${BLUE}Running Quick Demo...${NC}"
        bash "${SCRIPT_DIR}/quick_demo.sh"
        ;;
    2)
        echo ""
        echo -e "${BLUE}Running Full Test Suite...${NC}"
        bash "${SCRIPT_DIR}/run_all_tests.sh"
        ;;
    *)
        echo -e "${YELLOW}Invalid choice. Defaulting to Quick Demo.${NC}"
        bash "${SCRIPT_DIR}/quick_demo.sh"
        ;;
esac

# Step 3: Generate graphs
echo ""
echo -e "${YELLOW}Step 3: Generating graphs and analysis...${NC}"
python3 "${SCRIPT_DIR}/generate_graphs.py"

# Step 4: Summary
echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✓ Evaluation Complete!                                   ║${NC}"
echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo ""
echo -e "${GREEN}Results Location:${NC}"
echo "  CSV Data:  ${APP_DIR}/results/*.csv"
echo "  Logs:      ${APP_DIR}/results/*.log"
echo "  Graphs:    ${APP_DIR}/results/graphs/*.png"
echo "  Summary:   ${APP_DIR}/results/graphs/summary_report.txt"
echo ""
echo -e "${GREEN}View Graphs:${NC}"
echo "  cd ${APP_DIR}/results/graphs"
echo "  xdg-open *.png  # Or your preferred image viewer"
echo ""
echo -e "${GREEN}Read Summary:${NC}"
echo "  cat ${APP_DIR}/results/graphs/summary_report.txt"
echo ""
