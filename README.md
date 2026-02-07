# Parallel Count-Min Sketch

A comprehensive study of parallel Count-Min Sketch implementations using MPI, hybrid MPI+OpenMP and OpenMP on HPC clusters.

## Overview

This project implements and benchmarks different parallelization strategies for the Count-Min Sketch probabilistic data structure. The study compares pure MPI, hybrid MPI+OpenMP and pure OpenMP approaches on datasets up to 1 billion elements.

## Implementations

### MPI Versions

- **MPI Version 1 (`mpiV1.c`)**: Uses `MPI_Scatterv` for data distribution
- **MPI Version 2 (`mpiV2.c`)**: Uses parallel file I/O with `MPI_File_read_at`
  - Supports `pack` and `scatter` modes with optional `:excl` binding

### Hybrid MPI+OpenMP Versions

- **Hybrid V1 (`hybridV1.c`)**: Thread-private CMS copies (best hybrid performance)
- **Hybrid V2 (`hybridV2.c`)**: Shared CMS with atomic operations (poor scaling)
- **Hybrid V3 (`hybridV3.c`)**: Alternative hybrid implementation

### OpenMP Versions

- **OpenMP V1 (`openmpV1.c`)**: Thread-private CMS copies
- **OpenMP V2 (`openmpV2.c`)**: Shared CMS with atomic operations

### Serial Version

- **Serial (`cms_linear.c`)**: Baseline sequential implementation

## Project Structure

```
Parallel-Count-Min-Sketch/
├── csv_results              # Benchmark results
├── data                     # Test datasets
├── env                      # Environment configuration
├── plots                    # Generated visualizations
├── scripts                  # Data generation and analysis
├── src                      # Source code containing the implementations
├── benchmark_metrics.py     # Benchmark analysis and plotting
├── hybrid_benchmark.py      # Benchmarks for hybrid implementations
├── mpi_benchmark.py         # Benchmarks for MPI implementations
├── omp_benchmark.py         # Benchmarks for OpenMP implementations
├── *.sh                     # Execution scripts
├── Makefile                 # Build configuration
├── README.md                # Project documentation
```

## Building

### Prerequisites

- MPI implementation
- GCC with OpenMP support
- Python 3.x for analysis scripts
- PBS for cluster job submission

### Compilation

**MPI Version:**

```bash
mpicc -g -Wall -std=c99 -o mpiV2 count_min_sketch.c mpiV2.c -lm
```

**Hybrid Version:**

```bash
mpicc -g -Wall -std=c99 -fopenmp -o hybridV1 count_min_sketch_hybridV1.c hybridV1.c -lm
```

**OpenMP Version:**

```bash
gcc -g -Wall -std=c99 -fopenmp -o openmpV1 count_min_sketch.c openmpV1.c -lm
```

Or use the provided Makefile:

```bash
make
```

## Running

### MPI Execution

```bash
mpirun -np 4 ./mpiV2 data/dataset_250m.txt
```

Or use the provided script:

```bash
./run_mpiV2.sh
```

### Hybrid Execution

```bash
mpirun -np 2 ./hybridV1 data/dataset_250m.txt 4
# 2 MPI processes × 4 OpenMP threads = 8 total cores
```

Or use the provided script:

```bash
./run_hybridV1.sh
```

### OpenMP Execution

```bash
export OMP_NUM_THREADS=8
./openmpV1 data/dataset_250m.txt
```

Or use the provided script:

```bash
./run_openmp.sh
```

### Cluster Submission (PBS)

For cluster execution with job scheduler:

```bash
qsub run_mpi.sh
qsub run_hybrid.sh
qsub run_openmp.sh
```

## Benchmarking

### Generate Datasets

```bash
cd scripts
python gen_datasets.py
# Generates 250M, 500M, and 1000M element datasets
```

### Run Benchmarks

```bash
./run_benchmark.sh
```

This executes comprehensive benchmarks across:

- Dataset sizes: 250M, 500M, 1000M elements
- MPI processes: 1, 2, 4, 8, 16, 32, 64
- Thread counts: 1, 2, 4, 8, 16
- Modes: pack, scatter, pack:excl, scatter:excl

### Analyze Results

The scripts used to benchmark the implementations are the following:

- `mpi_benchmark.py`: Benchmarks the MPI implementations
- `hybrid_benchmark.py`: Benchmarks the hybrid MPI+OpenMP implementations
- `openmp_benchmark.py`: Benchmarks the OpenMP implementations

The benchmark scripts expect the executables to be placed in the root directory. The results are saved in `csv_results/`.

The script:

```bash
python benchmark_metrics.py      # Compute speedup and efficiency
```

can also be used to benchmark the implementations.
