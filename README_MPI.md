# Sparse Matrix-Vector Multiplication (SpMV) with MPI - Reproducibility Guide

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C](https://img.shields.io/badge/C-99-blue.svg)](https://en.cppreference.com/w/c)
[![MPI](https://img.shields.io/badge/MPI-MPICH-green.svg)](https://www.mpich.org/)

## Table of Contents

- [Overview](#overview)
- [MPI Implementation Details](#mpi-implementation-details)
- [Requirements](#requirements)
- [Installation](#installation)
- [Building the MPI Implementation](#building-the-mpi-implementation)
- [Usage](#usage)
    - [Running on HPC Cluster](#running-on-hpc-cluster)
- [MPI Configuration](#mpi-configuration)
- [Reproducibility Steps](#reproducibility-steps)
- [Performance Analysis](#performance-analysis)
- [Expected Output Structure](#expected-output-structure)
- [Environment Details](#environment-details)
- [Notes](#notes)
- [Contributors](#contributors)
- [Related Documentation](#related-documentation)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Overview

This document provides detailed reproducibility instructions for the **MPI-based Sparse Matrix-Vector Multiplication (SpMV)** implementation. The MPI version extends the OpenMP implementation to enable distributed memory parallelism across multiple compute nodes.

The MPI implementation:
- Distributes sparse matrix rows across MPI ranks
- Performs parallel SpMV computation using CSR format
- Compares performance against serial and OpenMP baselines
- Supports both file-based and synthetic matrix inputs
- Scales from 1 to 256 MPI processes

## MPI Implementation Details

### Communication Strategy

The implementation uses **1D row-wise partitioning**:
- Matrix rows are distributed among MPI ranks
- Each rank stores its local portion of the matrix in CSR format
- Vector elements are distributed according to row ownership
- Result vector is gathered at the root process (rank 0)

### Data Distribution

```
Matrix A (rows distributed):
Rank 0: rows [0, rows/P)
Rank 1: rows [rows/P, 2*rows/P)
...
Rank P-1: rows [(P-1)*rows/P, rows)

Vector x (broadcast to all ranks):
All ranks hold a complete copy of the input vector
```

### Source Code Structure

```
src/mpi/
├── spvm_mpi.c       # Main MPI implementation and entry point
├── spvm_mpi.h       # MPI-specific SpMV function declarations
├── spvm.c           # Core SpMV computation functions
├── spvm.h           # SpMV data structures (CSR, COO, Vector)
├── io.c             # Matrix Market file I/O
├── io.h
├── logger.c         # Logging and timing utilities
├── logger.h
├── exporter.c       # CSV results export
└── exporter.h
```

## Requirements

### System Requirements

- **OS**: Linux (tested on CentOS 7)
- **CPU**: Multi-core processor with MPI support
- **Network**: High-speed interconnect (InfiniBand recommended for multi-node)
- **RAM**: Depends on matrix size and number of MPI ranks

### Software Dependencies

#### MPI Implementation

```bash
# MPICH 3.2.1 or higher (recommended)
mpicc --version

# Or OpenMPI 4.0+
mpicc --version
```

#### C Compiler

```bash
# GCC 9.1.0 or higher
gcc-9.1.0 --version
```

#### Python (for analysis)

```bash
# Python 3.10 or higher
python3 --version

# Required packages
pip install numpy matplotlib pandas scipy
```

#### HPC Cluster Software

- PBS/Torque job scheduler (for cluster execution)

## Installation

### 1. Clone the Repository

```bash
git clone git@github.com:Fb1234566/PARCO-Computing-2026-237810.git
cd deliverable1_2025_2026
```

### 2. Load Required Modules (HPC Cluster)

```bash
# Load GCC compiler
module load gcc91

# Load MPI implementation
module load mpich-3.2.1--gcc-9.1.0

# Load Python for analysis
module load python-3.10.14

# Verify modules are loaded
module list
```

### 3. Verify MPI Installation

```bash
# Check MPI compiler
mpicc --version

# Check MPI runtime
mpirun --version

# Test MPI functionality
mpirun -np 2 hostname
```

### 4. Set Up Python Environment

```bash
# Create virtual environment
python3 -m venv .venv

# Activate virtual environment
source .venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install numpy matplotlib pandas scipy contourpy cycler fonttools kiwisolver packaging pillow pyparsing python-dateutil pytz tzdata
```

## Building the MPI Implementation

### Build MPI Executable

```bash
# Clean previous builds
make clean

# Build MPI binary
make mpi

# Build supporting executables (serial and OpenMP baselines)
make serial-only
make openmp-static-only
```

### Verify Build

```bash
# Check that the MPI binary exists
ls -lh bin/mpi

# The binary should be executable and located at: bin/mpi
```

### Build Options

| Target          | Description                | Output Binary        |
|-----------------|----------------------------|----------------------|
| `mpi`           | MPI implementation         | `bin/mpi`            |
| `serial-only`   | Serial baseline            | `bin/serial_spmv`    |
| `openmp-static-only` | OpenMP baseline       | `bin/openmp_spmv`    |

### Compiler Flags

The MPI implementation uses:
- MPI compiler wrapper: `mpicc`

## Usage

### Running on HPC Cluster


#### Submit Batch Jobs for All Matrices

The `Submit_jobs_mpi.sh` script automates job submission for all datasets:

```bash
# Make the script executable (first time only)
chmod +x Submit_jobs_mpi.sh

# Submit jobs for all matrices
./Submit_jobs_mpi.sh
```

**What the script does:**
1. Loads required modules (gcc, MPICH, Python)
2. Builds all necessary binaries
3. Creates timestamped result directories
4. Submits one PBS job per matrix in `datasets/` folder
5. Each job tests all MPI process counts: 1, 2, 4, 8, 16, 32, 64, 128, 256

**PBS Job Configuration (Run_MPI.pbs):**
```bash
#PBS -q short_cpuQ                                  # Queue name
#PBS -l walltime=06:00:00                           # 6 hours max
#PBS -l select=4:ncpus=64:mpiprocs=256:mem=25gb    # Resource allocation
```

**Resource Allocation:**
- 4 compute nodes
- 64 CPUs per node
- Up to 256 MPI processes total (64 × 4)
- 25 GB memory per node

## MPI Configuration

The implementation uses default MPI process placement optimized by PBS job scheduler.

### Environment Variables

The batch submission script automatically configures the following environment variables:

```bash
# OpenMP settings (if using hybrid MPI+OpenMP)
export OMP_PROC_BIND=close
export OMP_PLACES=cores
export OMP_DYNAMIC=false
export OMP_WAIT_POLICY=active
```


## Reproducibility Steps

### Complete MPI Test Reproduction

Follow these steps to fully reproduce the MPI benchmarks on the HPC cluster:

#### Step 1: Environment Setup

```bash
# Clone repository
git clone git@github.com:Fb1234566/PARCO-Computing-2026-237810.git
cd deliverable1_2025_2026

# Load modules (on HPC cluster)
module load gcc91
module load mpich-3.2.1--gcc-9.1.0
module load python-3.10.14

# Verify environment
mpicc --version
gcc --version
python3 --version
```

#### Step 2: Build Executables

```bash
# Clean previous builds
make clean

# Build all required binaries
make mpi
make serial-only
make openmp-static-only

# Verify builds
ls -lh bin/mpi
ls -lh bin/serial_spmv
ls -lh bin/openmp_spmv
```

#### Step 3: Prepare Datasets

```bash
# Download benchmark matrices (if not already present)
make datasets

# Verify datasets
ls -lh datasets/*.mtx
```

#### Step 4: Run Benchmarks (HPC Cluster)

```bash
# Submit all jobs to the cluster
./Submit_jobs_mpi.sh

# Monitor job status
qstat -u $USER

# Check job output
tail -f name.o
```

The `Submit_jobs_mpi.sh` script will:
1. Load required modules (gcc, MPICH, Python)
2. Build all necessary binaries
3. Create timestamped result directories
4. Submit one PBS job per matrix in `datasets/` folder
5. Each job tests all MPI process counts: 1, 2, 4, 8, 16, 32, 64, 128, 256
6. Run serial and OpenMP baselines for comparison
7. Execute 10 iterations per configuration for statistical robustness

#### Step 5: Analyze Results

After all jobs complete, analyze the results:

```bash
# Activate Python environment
source .venv/bin/activate

# Run analysis scripts
./Run_MPI_analysis.sh results/run_<timestamp> plots

# Deactivate environment
deactivate
```

#### Step 6: Verify Output

```bash
# Check CSV results
ls -lh results/run_<timestamp>/*/*.csv

# Check generated plots
ls -lh plots/run_<timestamp>/*/*.png

# View example speedup plot
display plots/run_<timestamp>/inline_1/plot_speedup_inline_1.png
```


## Performance Analysis

### Metrics Collected

For each MPI execution, the following metrics are recorded:

- **Time**: Total wall-clock time for SpMV computation (seconds)
- **Iteration**: Test iteration number (1-10 for statistical robustness)
- **Status**: Execution status indicator (success/failure)
- **NProc**: Number of MPI processes utilized
- **FLOP**: Total floating-point operations (2 × nnz for SpMV)
- **CommTime**: Time spent in MPI communication operations (seconds)
- **Overhead**: Communication overhead as percentage of total time
- **GFLOPS**: Performance in gigaflops per second (FLOP / Time / 10⁹)

### Strong and Weak Scaling

- **Strong Scaling**: Fixed problem size, varying process count (1, 2, 4, 8, 16, 32, 64, 128, 256)
- **Weak Scaling**: Problem size proportional to process count (base: 10,000 × 10,000 matrix with 200,000 nnz)

### Output Files

MPI results are saved in CSV format:

```
results/run_<timestamp>/<matrix_name>/
├── stats_MPI_<matrix_name>.csv          # MPI results
├── stats_Serial_<matrix_name>.csv       # Serial baseline
└── stats_openMP_Binning_<matrix_name>.csv  # OpenMP comparison
```

### Running Analysis Scripts

After collecting benchmark results, you can analyze the performance using the provided analysis scripts.

#### Automated Analysis (Recommended)

Run all analysis scripts at once using the master script:

```bash
# Basic usage - specify the timestamped results directory
./Run_MPI_analysis.sh results/run_20251116_142143 plots

# Alternative - with both directories explicitly specified
./Run_MPI_analysis.sh results/run_20251116_142143 plots/analysis_output

# If you want to use ./results as default, it will search for the latest run_* directory
./Run_MPI_analysis.sh
```

**Important**: The `results_dir` argument should point directly to your timestamped results directory (e.g., `results/run_20251116_142143`), which contains the matrix subdirectories.

The script will automatically:
1. Analyze all regular matrix directories for communication overhead
2. Perform strong scaling comparison between the first two matrices found
3. Execute basic weak scaling analysis (if `matrix_weak_scaling_*` directories exist)
4. Generate enhanced weak scaling reports with detailed metrics

**Example Output:**
```
========================================
   MPI Performance Analysis Suite
========================================

✓ Output directory: ./plots

Configuration:
  Results directory: results/run_20251116_142143
  Output directory:  plots
  Datasets directory: ./datasets
  Percentile filter: 90th

Found:
  Regular matrices: 3
  Weak scaling matrices: 9

[1/4] Running Communication Overhead Analysis...
  Analyzing: inline_1
  Analyzing: largebasis
  Analyzing: nd24k

[2/4] Running Strong Scaling Analysis...
  Comparing: inline_1 vs largebasis

[3/4] Running Basic Weak Scaling Analysis...
  Analyzing 9 weak scaling matrices...

[4/4] Running Enhanced Weak Scaling Analysis...
  Generating enhanced weak scaling report...

========================================
   Analysis Complete
========================================

✓ Results saved to: plots

Generated files:
  Plots:   12
  Reports: 2
  CSV:     4
```

#### Individual Analysis Scripts

You can also run individual analysis scripts for specific analyses:

**1. Communication Overhead Analysis**

Analyzes MPI communication overhead as a function of processor count:

```bash
python3 scripts/mpi/analyze_communication_overhead.py \
    results/run_20251116_142143/inline_1 \
    plots \
    90
```

**Arguments:**
- `matrix_folder`: Path to the matrix results directory
- `output_dir`: Output directory for plots (optional, default: current directory)
- `percentile`: Percentile threshold for filtering outliers (optional, default: 80)

**Outputs:**
- `plot_comm_overhead_<matrix_name>.png`: Communication overhead vs processor count
- Console summary with statistics

**2. Strong Scaling Analysis**

Compares strong scaling performance between two matrices:

```bash
python3 scripts/mpi/analyze_strong_scaling.py \
    results/run_20251116_142143 \
    inline_1 \
    largebasis \
    plots \
    90
```

**Arguments:**
- `data_path`: Directory containing matrix subdirectories
- `matrix1`: First matrix name (folder name)
- `matrix2`: Second matrix name (folder name)
- `output_dir`: Output directory for plots (optional, default: current directory)
- `percentile`: Percentile threshold (optional, default: 80)

**Outputs:**
- `plot_strong_scaling_comparison.png`: Side-by-side speedup comparison
- Detailed analysis for each matrix

**3. Basic Weak Scaling Analysis**

Analyzes weak scaling performance across serial, OpenMP, and MPI implementations:

```bash
python3 scripts/mpi/analyze_weak_scaling.py \
    results/run_20251116_142143 \
    plots \
    90
```

**Arguments:**
- `data_path`: Directory containing `matrix_weak_scaling_*` subdirectories
- `output_dir`: Output directory for plots (optional, default: current directory)
- `percentile`: Percentile threshold (optional, default: 80)

**Outputs:**
- `plot_weak_scaling.png`: Execution time vs problem size for all implementations
- Console summary with efficiency metrics

**4. Enhanced Weak Scaling Analysis**

Generates a comprehensive weak scaling report with detailed metrics:

```bash
python3 scripts/mpi/analyze_weak_scaling_spmv_enhanced.py \
    results/run_20251116_142143 \
    datasets \
    plots \
    90
```

**Arguments:**
- `data_path`: Directory containing `matrix_weak_scaling_*` subdirectories
- `datasets_dir`: Directory containing `.mtx` matrix files
- `output_dir`: Output directory for reports (optional, default: current directory)
- `percentile`: Percentile threshold (optional, default: 90)

**Outputs:**
- `weak_scaling_spmv_enhanced_report.md`: Comprehensive markdown report
- `mpi_weak_scaling_processed.csv`: Processed MPI data
- `omp_weak_scaling_processed.csv`: Processed OpenMP data
- Console summary with efficiency analysis

#### Requirements for Analysis Scripts

Ensure you have the required Python packages:

```bash
pip install pandas numpy matplotlib scipy
```

Or use the provided requirements file (if available):

```bash
pip install -r requirements.txt
```

### Generated Plots

Analysis scripts generate:

- **Speedup plots**: `plot_speedup_<matrix_name>.png`
- **Strong scaling plots**: `plot_strong_scaling_<matrix_name>.png`
- **Weak scaling plots**: `plot_weak_scaling_<matrix_name>.png`
- **Communication overhead plots**: `plot_comm_overhead_<matrix_name>.png`

## Expected Output Structure

After running MPI benchmarks, results are organized as follows:

```
results/
  └── run_<timestamp>/              # Timestamped run directory
      ├── inline_1/
      │   ├── stats_MPI_inline_1.csv
      │   ├── stats_Serial_inline_1.csv
      │   └── stats_openMP_Binning_inline_1.csv
      ├── nd24k/
      │   ├── stats_MPI_nd24k.csv
      │   ├── stats_Serial_nd24k.csv
      │   └── stats_openMP_Binning_nd24k.csv
      └── largebasis/
          ├── stats_MPI_largebasis.csv
          ├── stats_Serial_largebasis.csv
          └── stats_openMP_Binning_largebasis.csv

plots/
  └── run_<timestamp>/              # Timestamped plot directory
      ├── inline_1/
      │   ├── plot_speedup_inline_1.png
      │   ├── plot_strong_scaling_inline_1.png
      │   └── plot_comm_overhead_inline_1.png
      ├── nd24k/
      │   ├── plot_speedup_nd24k.png
      │   ├── plot_strong_scaling_nd24k.png
      │   └── plot_comm_overhead_nd24k.png
      └── largebasis/
          ├── plot_speedup_largebasis.png
          ├── plot_strong_scaling_largebasis.png
          └── plot_comm_overhead_largebasis.png

# PBS job output files
name.o                               # Standard output
name.e                               # Standard error
```

### CSV File Format

**MPI Results (stats_MPI_<matrix>.csv):**
```csv
Time,Iteration,Status,NProc,FLOP,CommTime,Overhead,GFLOPS
2.543,1,success,4,500000,0.125,0.0491,0.196
2.538,2,success,4,500000,0.127,0.0500,0.197
2.541,3,success,4,500000,0.126,0.0496,0.197
...
```

**Field Descriptions:**
- **Time**: Total execution time in seconds (wall-clock time)
- **Iteration**: Test iteration number (typically 1-10)
- **Status**: Execution status (success/failure)
- **NProc**: Number of MPI processes
- **FLOP**: Total floating-point operations performed (2 × nnz for SpMV)
- **CommTime**: MPI communication time in seconds
- **Overhead**: Communication overhead ratio (CommTime/Time)
- **GFLOPS**: Performance in gigaflops per second

**Serial Baseline (stats_Serial_<matrix>.csv):**
```csv
Time,Iteration,Status,Threads,FLOP,GFLOPS
5.123,1,success,1,500000,0.098
5.118,2,success,1,500000,0.098
...
```

**OpenMP Comparison (stats_openMP_Binning_<matrix>.csv):**
```csv
Time,Iteration,Status,Threads,FLOP,GFLOPS
1.245,1,success,8,500000,0.401
1.243,2,success,8,500000,0.402
...
```

## Environment Details

### HPC System Specification

All MPI benchmarks were executed on the following HPC system:

#### System Information

- **OS**: CentOS Linux 7 (Core)
- **Architecture**: x86_64

#### Hardware Specification

- **CPU Model**: Intel(R) Xeon(R) Gold 6252N CPU @ 2.30GHz
- **Total Cores**: 96 (24 cores per socket × 4 sockets)
- **Threads per Core**: 1 (Hyper-Threading disabled)
- **Base Frequency**: 2.30 GHz
- **NUMA Nodes**: 4
    - NUMA node0: CPUs 0-23
    - NUMA node1: CPUs 24-47
    - NUMA node2: CPUs 48-71
    - NUMA node3: CPUs 72-95

- **Cache Hierarchy**:
    - L1d cache: 32 KB per core
    - L1i cache: 32 KB per core
    - L2 cache: 1024 KB (1 MB) per core
    - L3 cache: 36608 KB (~35.75 MB) shared per socket

- **Memory**: High-memory HPC node configuration
- **Architecture Features**: AVX-512, TSX, RDRAND, SGX

#### Software Stack

**MPI Implementation**:
- MPICH 3.2.1 with GCC 9.1.0 bindings
- MPI Runtime: mpirun version 3.2.1
- MPI Standard: MPI-3.0 compliant

**Compiler**:
- GCC 9.1.0 with C++17 support
- OpenMP API Version: 3.1 (201107)
- Optimization: -O2

**Python Environment**:
- Python 3.10.14

**Core Scientific Libraries**:
- numpy 2.2.6
- matplotlib 3.10.7
- pandas 2.3.3
- scipy 1.15.3

**Supporting Libraries**:
- contourpy 1.3.2
- cycler 0.12.1
- fonttools 4.60.1
- kiwisolver 1.4.9
- packaging 25.0
- pillow 12.0.0
- pyparsing 3.2.5
- python-dateutil 2.9.0.post0
- pytz 2025.2
- tzdata 2025.2

**Job Scheduler**:
- PBS/Torque

### Compilation Details

```bash
# MPI Compiler
mpicc --version
# gcc (GCC) 9.1.0

# Compilation command
mpicc src/mpi/*.c -o bin/mpi
```
## Notes

- **Optimization Levels**: All implementations (Serial, OpenMP, MPI) use `-O2` for fair comparison
- **Process Scaling**: Test process counts from 1 to 256 (powers of 2) as per experimental setup
- **Statistical Robustness**: 10 iterations per configuration; 90th percentile used to reduce noise impact
- **Memory Requirements**: High-memory HPC node configuration with 96 cores
- **Network**: Multi-node performance heavily depends on interconnect speed
- **Reproducibility**: 
  - Use identical compiler versions (GCC 9.1.0, MPICH 3.2.1)
  - Use identical optimization flags (`-O2`)
  - Run same number of iterations (10)
  - Use same statistical metric (90th percentile)
- **Matrix Selection**: Strong scaling uses 5 datasets with high variability and coefficient of variation
- **Weak Scaling**: Base matrix (10,000 × 10,000, 200,000 nnz) scales proportionally with process count

## Contributors

- Filippo Benedetti
- Student ID: 237810
- Course: Introduction to Parallel Computing 2025/2026
- Institution: University of Trento

## Related Documentation

- [Main README](README.md) - OpenMP implementation and general project overview
- [Run_MPI.pbs](Run_MPI.pbs) - PBS job configuration
- [Submit_jobs_mpi.sh](Submit_jobs_mpi.sh) - Batch job submission script
- [Run_MPI_analysis.sh](Run_MPI_analysis.sh) - Automated analysis script

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- MPICH Development Team for the MPI implementation
- SuiteSparse Matrix Collection for providing benchmark datasets
- HPC cluster administrators for computational resources
- Course instructors and teaching assistants


