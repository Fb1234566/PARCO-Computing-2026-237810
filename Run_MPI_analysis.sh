#!/bin/bash
################################################################################
# Run MPI Analysis Scripts
#
# This script executes all MPI analysis scripts in sequence to generate
# performance plots and reports.
#
# Usage:
#   ./Run_MPI_analysis.sh [results_dir] [output_dir]
#
# Arguments:
#   results_dir  : Directory containing benchmark results (default: ./results)
#   output_dir   : Output directory for plots and reports (default: ./plots)
#
# Requirements:
#   - Python 3.x with pandas, numpy, matplotlib
#   - Benchmark results from MPI runs
#   - Matrix files in ./datasets directory
################################################################################

set -e  # Exit on error

# Configuration
RESULTS_DIR="${1:-./results}"
OUTPUT_DIR="${2:-./plots}"
DATASETS_DIR="./datasets"
SCRIPTS_DIR="./scripts/mpi"
PERCENTILE=90

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Banner
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   MPI Performance Analysis Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Validate directories
if [ ! -d "$RESULTS_DIR" ]; then
    echo -e "${RED}Error: Results directory '$RESULTS_DIR' not found${NC}"
    exit 1
fi

if [ ! -d "$SCRIPTS_DIR" ]; then
    echo -e "${RED}Error: Scripts directory '$SCRIPTS_DIR' not found${NC}"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
echo -e "${GREEN}✓${NC} Output directory: $OUTPUT_DIR"
echo ""

# Find latest results directory if results_dir contains timestamped runs
if [ -d "$RESULTS_DIR" ]; then
    LATEST_RUN=$(find "$RESULTS_DIR" -maxdepth 1 -type d -name "run_*" | sort -r | head -n 1)
    if [ -n "$LATEST_RUN" ]; then
        echo -e "${YELLOW}Found timestamped results in: $LATEST_RUN${NC}"
        RESULTS_DIR="$LATEST_RUN"
    fi
fi

echo -e "${BLUE}Configuration:${NC}"
echo -e "  Results directory: $RESULTS_DIR"
echo -e "  Output directory:  $OUTPUT_DIR"
echo -e "  Datasets directory: $DATASETS_DIR"
echo -e "  Percentile filter: ${PERCENTILE}th"
echo ""

################################################################################
# 1. Communication Overhead Analysis
################################################################################
echo -e "${BLUE}[1/4] Running Communication Overhead Analysis...${NC}"

# Find matrix folders with MPI results
MATRIX_FOLDERS=$(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d)

if [ -z "$MATRIX_FOLDERS" ]; then
    echo -e "${YELLOW}Warning: No matrix folders found in $RESULTS_DIR${NC}"
else
    for MATRIX_FOLDER in $MATRIX_FOLDERS; do
        MATRIX_NAME=$(basename "$MATRIX_FOLDER")

        # Check if MPI stats exist
        if ls "$MATRIX_FOLDER"/stats_*mpi*.csv 1> /dev/null 2>&1; then
            echo -e "  Analyzing: ${GREEN}$MATRIX_NAME${NC}"
            python3 "$SCRIPTS_DIR/analyze_communication_overhead.py" \
                "$MATRIX_FOLDER" \
                "$OUTPUT_DIR" \
                "$PERCENTILE" || echo -e "${YELLOW}  Warning: Failed to analyze $MATRIX_NAME${NC}"
        fi
    done
fi
echo ""

################################################################################
# 2. Strong Scaling Analysis
################################################################################
echo -e "${BLUE}[2/4] Running Strong Scaling Analysis...${NC}"

# Find pairs of matrices to compare
AVAILABLE_MATRICES=($(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d -exec basename {} \;))

if [ ${#AVAILABLE_MATRICES[@]} -ge 2 ]; then
    echo -e "  Comparing: ${GREEN}${AVAILABLE_MATRICES[0]}${NC} vs ${GREEN}${AVAILABLE_MATRICES[1]}${NC}"
    python3 "$SCRIPTS_DIR/analyze_strong_scaling.py" \
        "$RESULTS_DIR" \
        "${AVAILABLE_MATRICES[0]}" \
        "${AVAILABLE_MATRICES[1]}" \
        "$OUTPUT_DIR" \
        "$PERCENTILE" || echo -e "${YELLOW}  Warning: Strong scaling analysis failed${NC}"
else
    echo -e "${YELLOW}  Warning: Need at least 2 matrices for strong scaling comparison${NC}"
fi
echo ""

################################################################################
# 3. Weak Scaling Analysis (Basic)
################################################################################
echo -e "${BLUE}[3/4] Running Basic Weak Scaling Analysis...${NC}"

# Check for weak scaling matrices
WEAK_SCALING_FOLDERS=$(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d -name "matrix_weak_scaling_*" | head -n 1)

if [ -n "$WEAK_SCALING_FOLDERS" ]; then
    echo -e "  Analyzing weak scaling results from: ${GREEN}$RESULTS_DIR${NC}"
    python3 "$SCRIPTS_DIR/analyze_weak_scaling.py" \
        "$RESULTS_DIR" \
        "$OUTPUT_DIR" \
        "$PERCENTILE" || echo -e "${YELLOW}  Warning: Weak scaling analysis failed${NC}"
else
    echo -e "${YELLOW}  Warning: No weak scaling matrices found (matrix_weak_scaling_*)${NC}"
fi
echo ""

################################################################################
# 4. Enhanced Weak Scaling Analysis
################################################################################
echo -e "${BLUE}[4/4] Running Enhanced Weak Scaling Analysis...${NC}"

if [ -d "$DATASETS_DIR" ] && [ -n "$WEAK_SCALING_FOLDERS" ]; then
    echo -e "  Generating enhanced weak scaling report..."
    python3 "$SCRIPTS_DIR/analyze_weak_scaling_spmv_enhanced.py" \
        "$RESULTS_DIR" \
        "$DATASETS_DIR" \
        "$OUTPUT_DIR" \
        "$PERCENTILE" || echo -e "${YELLOW}  Warning: Enhanced weak scaling analysis failed${NC}"
else
    if [ ! -d "$DATASETS_DIR" ]; then
        echo -e "${YELLOW}  Warning: Datasets directory '$DATASETS_DIR' not found${NC}"
    fi
    if [ -z "$WEAK_SCALING_FOLDERS" ]; then
        echo -e "${YELLOW}  Warning: No weak scaling results found${NC}"
    fi
fi
echo ""

################################################################################
# Summary
################################################################################
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Analysis Complete${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "${GREEN}✓${NC} Results saved to: $OUTPUT_DIR"
echo ""

# List generated files
if [ -d "$OUTPUT_DIR" ]; then
    NUM_PLOTS=$(find "$OUTPUT_DIR" -name "*.png" 2>/dev/null | wc -l)
    NUM_REPORTS=$(find "$OUTPUT_DIR" -name "*.md" 2>/dev/null | wc -l)
    NUM_CSV=$(find "$OUTPUT_DIR" -name "*.csv" 2>/dev/null | wc -l)

    echo -e "${BLUE}Generated files:${NC}"
    echo -e "  Plots:   $NUM_PLOTS"
    echo -e "  Reports: $NUM_REPORTS"
    echo -e "  CSV:     $NUM_CSV"
    echo ""

    # Show some example outputs
    if [ $NUM_PLOTS -gt 0 ]; then
        echo -e "${BLUE}Sample plots:${NC}"
        find "$OUTPUT_DIR" -name "*.png" 2>/dev/null | head -n 3 | while read -r file; do
            echo -e "  - $(basename "$file")"
        done
        echo ""
    fi

    if [ $NUM_REPORTS -gt 0 ]; then
        echo -e "${BLUE}Reports:${NC}"
        find "$OUTPUT_DIR" -name "*.md" 2>/dev/null | while read -r file; do
            echo -e "  - $(basename "$file")"
        done
        echo ""
    fi
fi

echo -e "${GREEN}Done!${NC}"

