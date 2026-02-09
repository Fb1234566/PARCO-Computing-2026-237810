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

echo -e "${BLUE}Configuration:${NC}"
echo -e "  Results directory: $RESULTS_DIR"
echo -e "  Output directory:  $OUTPUT_DIR"
echo -e "  Datasets directory: $DATASETS_DIR"
echo -e "  Percentile filter: ${PERCENTILE}th"
echo ""

# Count available analyses
MATRIX_COUNT=$(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d ! -name "matrix_weak_scaling_*" | wc -l)
WEAK_SCALING_COUNT=$(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d -name "matrix_weak_scaling_*" | wc -l)

echo -e "${BLUE}Found:${NC}"
echo -e "  Regular matrices: $MATRIX_COUNT"
echo -e "  Weak scaling matrices: $WEAK_SCALING_COUNT"
echo ""

################################################################################
# 1. Communication Overhead Analysis
################################################################################
echo -e "${BLUE}[1/4] Running Communication Overhead Analysis...${NC}"

# Find matrix folders with MPI results (excluding weak scaling matrices)
MATRIX_FOLDERS=$(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d ! -name "matrix_weak_scaling_*")

if [ -z "$MATRIX_FOLDERS" ]; then
    echo -e "${YELLOW}  No matrix folders found${NC}"
else
    for MATRIX_FOLDER in $MATRIX_FOLDERS; do
        MATRIX_NAME=$(basename "$MATRIX_FOLDER")

        # Check if MPI stats exist
        if ls "$MATRIX_FOLDER"/stats_*[Mm][Pp][Ii]*.csv 1> /dev/null 2>&1; then
            echo -e "  Analyzing: ${GREEN}$MATRIX_NAME${NC}"
            python3 "$SCRIPTS_DIR/analyze_communication_overhead.py" \
                "$MATRIX_FOLDER" \
                "$OUTPUT_DIR" \
                "$PERCENTILE" 2>&1 | grep -E "(Error|✓|Analyzing|Summary)" || echo -e "${YELLOW}    Warning: Analysis produced no output${NC}"
        else
            echo -e "  ${YELLOW}Skipping $MATRIX_NAME (no MPI results)${NC}"
        fi
    done
fi
echo ""

################################################################################
# 2. Strong Scaling Analysis
################################################################################
echo -e "${BLUE}[2/4] Running Strong Scaling Analysis...${NC}"

# Find pairs of matrices to compare (excluding weak scaling)
AVAILABLE_MATRICES=($(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d ! -name "matrix_weak_scaling_*" -exec basename {} \; | sort))

if [ ${#AVAILABLE_MATRICES[@]} -ge 2 ]; then
    echo -e "  Comparing: ${GREEN}inline_1${NC} vs ${GREEN}largebasis${NC}"
    python3 "$SCRIPTS_DIR/analyze_strong_scaling.py" \
        "$RESULTS_DIR" \
        "inline_1" \
        "largebasis" \
        "$OUTPUT_DIR" \
        "$PERCENTILE" 2>&1 | grep -E "(Error|✓|Analyzing|Plotting)" || echo -e "${YELLOW}    Warning: Analysis produced no output${NC}"
else
    echo -e "${YELLOW}  Skipped: Need at least 2 matrices (found ${#AVAILABLE_MATRICES[@]})${NC}"
fi
echo ""

################################################################################
# 3. Weak Scaling Analysis (Basic)
################################################################################
echo -e "${BLUE}[3/4] Running Basic Weak Scaling Analysis...${NC}"

# Check for weak scaling matrices
WEAK_SCALING_COUNT=$(find "$RESULTS_DIR" -mindepth 1 -maxdepth 1 -type d -name "matrix_weak_scaling_*" | wc -l)

if [ $WEAK_SCALING_COUNT -gt 0 ]; then
    echo -e "  Analyzing $WEAK_SCALING_COUNT weak scaling matrices..."
    python3 "$SCRIPTS_DIR/analyze_weak_scaling.py" \
        "$RESULTS_DIR" \
        "$OUTPUT_DIR" \
        "$PERCENTILE" 2>&1 | grep -E "(Error|✓|Analyzing|Loaded|Plotting)" || echo -e "${YELLOW}    Warning: Analysis produced no output${NC}"
else
    echo -e "${YELLOW}  Skipped: No weak scaling matrices found${NC}"
fi
echo ""

################################################################################
# 4. Enhanced Weak Scaling Analysis
################################################################################
echo -e "${BLUE}[4/4] Running Enhanced Weak Scaling Analysis...${NC}"

if [ $WEAK_SCALING_COUNT -gt 0 ] && [ -d "$DATASETS_DIR" ]; then
    echo -e "  Generating enhanced weak scaling report..."
    python3 "$SCRIPTS_DIR/analyze_weak_scaling_spmv_enhanced.py" \
        "$RESULTS_DIR" \
        "$DATASETS_DIR" \
        "$OUTPUT_DIR" \
        "$PERCENTILE" 2>&1 | grep -E "(Error|✓|Loading|Analyzing|Report)" || echo -e "${YELLOW}    Warning: Analysis produced no output${NC}"
else
    if [ $WEAK_SCALING_COUNT -eq 0 ]; then
        echo -e "${YELLOW}  Skipped: No weak scaling results${NC}"
    fi
    if [ ! -d "$DATASETS_DIR" ]; then
        echo -e "${YELLOW}  Skipped: Datasets directory '$DATASETS_DIR' not found${NC}"
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

echo -e "${GREEN}Done!${NC}"

