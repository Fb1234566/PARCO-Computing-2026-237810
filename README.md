# Sparse Matrix-Vector Multiplication (SpMV) - Introduction to Parallel Computing Project

[![License](https://img.shields.io/badge/License-Academic-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenMP](https://img.shields.io/badge/OpenMP-Enabled-green.svg)](https://www.openmp.org/)

## Table of Contents

- [Overview](#overview)
- [Project Structure](#project-structure)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Building the Project](#building-the-project)
- [Usage](#usage)
    - [Running Individual Implementations](#running-individual-implementations)
    - [Running Benchmarks](#running-benchmarks)
    - [Running Tests](#running-tests)
- [Dataset Management](#dataset-management)
- [Implementation Details](#implementation-details)
- [Performance Analysis](#performance-analysis)
- [Results](#results)
- [Reproducibility](#reproducibility)
- [Contributors](#contributors)

## Overview

This project implements and benchmarks multiple parallel approaches for **Sparse Matrix-Vector Multiplication (SpMV)**
using OpenMP. SpMV is a fundamental operation in scientific computing, appearing in iterative solvers, graph algorithms,
and machine learning applications.

The project compares:

- **Serial Implementation**: Baseline sequential SpMV
- **OpenMP Static Scheduling**: Parallel SpMV with static work distribution
- **OpenMP Dynamic Scheduling**: Parallel SpMV with dynamic work distribution
- **OpenMP Guided Scheduling**: Parallel SpMV with guided work distribution
- **OpenMP Binning**: Load-balanced parallel SpMV using row-binning strategy

All implementations use the **Compressed Sparse Row (CSR)** format for efficient sparse matrix storage and computation.

## Project Structure

```
.
├── src/
│   ├── interfaces/          # Abstract interfaces for SpMV implementations
│   │   └── SpVMInterface.h
│   ├── serial/              # Serial implementation
│   │   ├── SpMVSerial.h
│   │   ├── SpMVSerial.cpp
│   │   └── main_serial.cpp
│   ├── openMP/              # OpenMP parallel implementations
│   │   ├── SpMVOpenMPStatic.h/.cpp
│   │   ├── SpMVOpenMPDynamic.h/.cpp
│   │   ├── SpMVOpenMPGuided.h/.cpp
│   │   ├── SpMVOpenMPBinning.h/.cpp
│   │   └── main_openmp_*.cpp
│   └── utils/               # Utility classes and functions
│       ├── MatrixReader.h/.cpp      # Matrix Market format reader
│       ├── ExecutionStatistics.h/.cpp  
│       ├── PrintUtils.h/.cpp
│       └── DataTable.h/.cpp
├── scripts/                 # Python analysis scripts
│   ├── analyze_matrixes.py
│   ├── analyze_results.py
│   └── make_paper_figures.py
├── datasets/               # Sparse matrix datasets (Matrix Market format)
├── results/                # Execution results and statistics (CSV)
├── plots/                  # Generated performance plots
├── bin/                    # Compiled binaries
├── build/                  # Build artifacts (object files)
├── main.cpp               # Main entry point
├── Makefile               # Build system configuration
├── Run.sh                 # Main execution script
├── Run_tests.sh          # Testing and benchmarking script
├── Submit_jobs.sh        # PBS job submission script
├── Run.pbs               # PBS job configuration
├── datasets.txt          # List of datasets to download
└── README.md             # This file
```

## Features

- **Multiple Scheduling Strategies**: Compare different OpenMP scheduling approaches
- **Load Balancing**: Binning strategy for handling matrices with irregular row distributions
- **Comprehensive Benchmarking**: Automated execution with multiple thread counts
- **Statistical Analysis**: Performance metrics collection and CSV export
- **Visualization**: Python scripts for generating speedup and timing plots
- **Matrix Market Support**: Read standard sparse matrix formats
- **Correctness Verification**: Automatic validation against serial reference implementation
- **Reproducible Results**: Automated scripts for complete reproducibility

## Requirements

### System Requirements

- **OS**: Linux (tested on Ubuntu/Debian-based systems)
- **CPU**: Multi-core processor with OpenMP support
- **RAM**: Depends on matrix size (minimum 4GB recommended)

### Software Dependencies

#### C++ Compiler & Tools

```bash
# GCC with OpenMP support (version 7.0 or higher)
g++ --version  # Should show version 7.0+

# Make
make --version
```

#### Python (for analysis and plotting)

```bash
# Python 3.7 or higher
python3 --version

# Required packages
pip install numpy matplotlib pandas scipy
```

#### Optional (for HPC clusters)

- PBS/Torque job scheduler (if using Submit_jobs.sh)

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/PARCO-Computing-2026-237810.git
cd PARCO-Computing-2026-237810
```

### 2. Verify Dependencies

```bash
# Check GCC and OpenMP support
g++ -fopenmp --version

# Check Python installation
python3 --version
```

### 3. Set Up Python Environment (Optional but Recommended)

```bash
# Create virtual environment
python3 -m venv .venv

# Activate virtual environment
source .venv/bin/activate

# Install required packages
pip install --upgrade pip
pip install numpy matplotlib pandas scipy
```

## Building the Project

### Build All Implementations

```bash
# Clean previous builds
make clean

# Build all executables
make serial-only
make openmp-static-only
make openmp-dynamic-only
make openmp-guided-only
make openmp-binning-only
```

### Build Options

| Target                | Description               | Output Binary             |
|-----------------------|---------------------------|---------------------------|
| `serial-only`         | Serial implementation     | `bin/serial_spmv`         |
| `openmp-static-only`  | OpenMP static scheduling  | `bin/openmp_spmv`         |
| `openmp-dynamic-only` | OpenMP dynamic scheduling | `bin/openmp_spmv_dynamic` |
| `openmp-guided-only`  | OpenMP guided scheduling  | `bin/openmp_spmv_guided`  |
| `openmp-binning-only` | OpenMP with binning       | `bin/openmp_spmv_binning` |

### Compiler Flags

The project uses the following compilation flags:

- `-std=c++17`: C++17 standard
- `-Wall -Wextra -Wpedantic`: Enable all warnings
- `-O0 -g`: No optimization, debug symbols (change for production)
- `-fopenmp`: Enable OpenMP support

## Usage

### Running Individual Implementations

#### Serial Implementation

```bash
./bin/serial_spmv
```

#### OpenMP Implementations

```bash
# Static scheduling
./bin/openmp_spmv  <matrix_path> <current_iteration> <output_dir> <nthreads>

# Dynamic scheduling
./bin/openmp_spmv_dynamic <matrix_path> <current_iteration> <output_dir> <nthreads>

# Guided scheduling
./bin/openmp_spmv_guided <matrix_path> <current_iteration> <output_dir> <nthreads>

# Binning approach
./bin/openmp_spmv_binning <matrix_path> <current_iteration> <output_dir> <nthreads>
```

### Running Benchmarks

The `Run.sh` script automates execution across all implementations and thread counts for a single matrix:

```bash
# Make the script executable (first time only)
chmod +x Run.sh

# Run benchmarks
./Run.sh <matrix_path> <results_dir> <plots_dir>
```

The script will:

1. Automatically locate compiled binaries
2. Execute all implementations on all matrices in `datasets/`
3. Test multiple thread counts (1, 2, 4, 8, 16, 32, 64)
4. Save results to `results/` directory
5. Generate plots in `plots/` directory

### Running Tests

For a complete test run with compilation, execution, and analysis:

```bash
# Make the script executable (first time only)
chmod +x Run_tests.sh

# Run complete test suite
./Run_tests.sh
```

This script:

1. Cleans and rebuilds all implementations
2. Sets up Python virtual environment
3. Downloads datasets (if needed)
4. Executes all benchmarks
5. Generates performance analysis plots
6. Creates speedup and timing comparisons

### Running on HPC Cluster

For PBS/Torque clusters:

```bash
# Submit job to queue
qsub Run.pbs -v MATRIX=<matrix_path>,OUTPUT_DIR=<result_dir>,GRAPH_DIR=<graph_directory> Run.pbs
```

Or use the submission script:

```bash
./Submit_jobs.sh
```

## Dataset Management

### Downloading Datasets

Datasets are listed in `datasets.txt` and can be downloaded automatically using make:

```bash
make datasets
```

### Dataset Format

The project uses Matrix Market (`.mtx`) format. Datasets are sourced
from [SuiteSparse Matrix Collection](https://sparse.tamu.edu/).

Example datasets included:

- `inline_1.mtx` - Structural problem
- `nd24k.mtx` - 2D/3D Problem
- `largebasis.mtx` - Optimization problem
- `Ga41As41H72.mtx` - Quantum chemistry
- `pre2.mtx` - Frequency Domain Circuit Simulation Problem

### Adding Custom Datasets

1. Place `.mtx` files in the `datasets/` directory
2. The program will automatically process all matrices in this folder
3. Supported format: Matrix Market Coordinate Format

## Implementation Details

### CSR Format

All matrices are stored in Compressed Sparse Row (CSR) format:

```cpp
struct CSRMatrix {
    std::vector<double> values;      // Non-zero values
    std::vector<int> col_indices;    // Column indices
    std::vector<int> row_ptr;        // Row pointer array
    int num_rows;
    int num_cols;
    int num_nonzeros;
};
```

### Scheduling Strategies

1. **Static Scheduling**: Divide rows evenly among threads at compile time
2. **Dynamic Scheduling**: Assign rows to threads dynamically with small chunks
3. **Guided Scheduling**: Start with large chunks, decrease to improve load balance
4. **Binning**: Group rows by non-zero count for better load distribution

### Binning Algorithm

The binning approach:

1. Analyzes row non-zero distribution
2. Creates bins of rows with similar density
3. Distributes bins across threads for balanced workload
4. Particularly effective for matrices with irregular sparsity patterns

## Performance Analysis

### Metrics Collected

For each execution, the following metrics are recorded:

- **Execution Time**: Wall-clock time for SpMV computation (milliseconds)
- **Speedup**: Ratio of serial time to parallel time
- **Efficiency**: Speedup divided by number of threads
- **Thread Count**: Number of OpenMP threads used
- **Matrix Properties**: Rows, columns, non-zeros, sparsity

### Output Files

Results are saved in CSV format under `results/`:

```
stats_openMP_Static_SpMV_datasets_<matrix_name>.csv
stats_openMP_Dynamic_SpMV_datasets_<matrix_name>.csv
stats_openMP_Guided_SpMV_datasets_<matrix_name>.csv
stats_openMP_Binning_SpMV_datasets_<matrix_name>.csv
```

### Generated Plots

The analysis scripts generate:

- **Speedup plots**: `plot_speedup_<matrix_name>.png`
- **Timing plots**: `plot_time_<matrix_name>.png`
- **Log-log timing plots**: `plot_time_loglog_<matrix_name>.png`

## Results

### Performance Overview

| Matrix      | Rows    | Non-zeros  | Best Speedup | Best Strategy | Threads |
|-------------|---------|------------|--------------|---------------|---------|
| inline_1    | 503,712 | 18,660,027 |              |               | 32      |
| nd24k       | 72,000  | 14,220,946 |              |               | 32      |
| largebasis  | 504,855 | 4,617,816  |              |               | 32      |
| Ga41As41H72 | 268,096 | 18,488,476 |              |               | 32      |
| pre2        | 659,033 | 5,959,282  |              |               | 32      |

### Key Findings

- **Binning** strategy most effective for matrices with irregular row distribution
- **Guided** scheduling optimal for balanced workloads
- Near-linear scaling up to 32 threads, diminishing returns beyond 48
- Memory bandwidth becomes bottleneck at high thread counts

### Sample Performance Plot

![Speedup Comparison](docs/media/comparison_max_speedup.png)

## Reproducibility

### Complete Reproduction Steps

1. **Clone and setup**:
   ```bash
   git clone <repository-url>
   cd deliverable1_2025_2026
   ```

2. **Run complete test suite**:
   ```bash
   chmod +x Run_tests.sh
   ./Run_tests.sh
   ```

3. **Check results**:
   ```bash
   ls results/*.csv    # Performance data
   ls plots/*.png      # Visualization plots
   ```

### Environment Details

All benchmarks were executed on the following HPC system:

### System information:

- OS: CentOS Linux 7 (Core)
- Kernel: 3.10.0-1160.119.1.el7.x86_64
- Architecture: x86_64

### Hardware Specification

- CPU Model: Intel(R) Xeon(R) Gold 6252N CPU @ 2.30GHz
- Total Cores: 96 (24 cores per socket × 4 sockets)
- Threads per Core: 1 (Hyper-Threading disabled)
- Base Frequency: 2.30 GHz
- NUMA Nodes: 4
    - NUMA node0: CPUs 0-23
    - NUMA node1: CPUs 24-47
    - NUMA node2: CPUs 48-71
    - NUMA node3: CPUs 72-95

- Cache Hierarchy:
    - L1d cache: 32 KB per core
    - L1i cache: 32 KB per core
    - L2 cache: 1024 KB (1 MB) per core
    - L3 cache: 36608 KB (~35.75 MB) shared per socket

- Memory: High-memory HPC node configuration
- Architecture Features: AVX-512, TSX, RDRAND, SGX

### Software Stack:

**Compiler & Parallel Computing**:

- GCC 9.1.0 with C++17 support
- OpenMP API version: 3.1 (201107)

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

### Expected Output Structure

After running tests, results are organized by timestamp and matrix name:

```
results/
  └── run_<timestamp>/              # Timestamped run directory
      ├── inline_1/
      │   ├── stats_openMP_Static_SpMV_datasets_inline_1.csv
      │   ├── stats_openMP_Dynamic_SpMV_datasets_inline_1.csv
      │   ├── stats_openMP_Guided_SpMV_datasets_inline_1.csv
      │   └── stats_openMP_Binning_SpMV_datasets_inline_1.csv
      ├── nd24k/
      │   ├── stats_openMP_Static_SpMV_datasets_nd24k.csv
      │   ├── stats_openMP_Dynamic_SpMV_datasets_nd24k.csv
      │   ├── stats_openMP_Guided_SpMV_datasets_nd24k.csv
      │   └── stats_openMP_Binning_SpMV_datasets_nd24k.csv
      └── largebasis/
          ├── stats_openMP_Static_SpMV_datasets_largebasis.csv
          ├── stats_openMP_Dynamic_SpMV_datasets_largebasis.csv
          ├── stats_openMP_Guided_SpMV_datasets_largebasis.csv
          └── stats_openMP_Binning_SpMV_datasets_largebasis.csv

plots/
  └── run_<timestamp>/              # Timestamped plot directory
      ├── inline_1/
      │   ├── plot_speedup_inline_1.png
      │   ├── plot_time_inline_1.png
      │   └── plot_time_loglog_inline_1.png
      ├── nd24k/
      │   ├── plot_speedup_nd24k.png
      │   ├── plot_time_nd24k.png
      │   └── plot_time_loglog_nd24k.png
      └── largebasis/
          ├── plot_speedup_largebasis.png
          ├── plot_time_largebasis.png
          └── plot_time_loglog_largebasis.png

app.log                             # Execution log file
```

**Directory Structure**:

- `run_<timestamp>`: Each test run creates a unique directory (e.g., `run_20251116_140552`)
- `matrix_name/`: Subdirectories for each matrix tested
- CSV files contain performance metrics for each scheduling strategy
- PNG files contain visualization plots for each matrix

**Example**:

```bash
# After running ./Run_tests.sh
ls results/run_20251116_140552/inline_1/
# Output: stats_openMP_Static_SpMV_datasets_inline_1.csv
#         stats_openMP_Dynamic_SpMV_datasets_inline_1.csv
#         stats_openMP_Guided_SpMV_datasets_inline_1.csv
#         stats_openMP_Binning_SpMV_datasets_inline_1.csv

ls plots/run_20251116_140552/inline_1/
# Output: plot_speedup_inline_1.png
#         plot_time_inline_1.png
#         plot_time_loglog_inline_1.png
```

## Notes

- **Optimization Flags**: Current Makefile uses `-O0` for debugging. For production benchmarks, change to `-O2`
- **Thread Scaling**: Test thread counts appropriate for your CPU (avoid oversubscription)
- **Memory**: Large matrices may require significant RAM
- **Verification**: All parallel implementations are verified against serial results
- **Reproducibility**: Fix random seed for deterministic vector generation

## Contributors

- Filippo Benedetti
- Student ID: 237810
- Course: Introduction to Parallel Computing 2025/2026
- Institution: University of Trento

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- SuiteSparse Matrix Collection for providing benchmark datasets
- OpenMP Architecture Review Board for parallel computing standards
- Course instructors and teaching assistants

---

