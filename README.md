# Parallel Count-Min Sketch

A comprehensive study of parallel Count-Min Sketch implementations using MPI, hybrid MPI+OpenMP and OpenMP on HPC clusters.

## Overview

This project implements and benchmarks different parallelization strategies for the Count-Min Sketch probabilistic data structure. The study compares pure MPI, hybrid MPI+OpenMP and pure OpenMP approaches on datasets up to 1 billion elements.

## Implementations

### MPI Versions
- **MPI Version 1 (`main.c`)**: Uses `MPI_Scatterv` for data distribution
- **MPI Version 2 (`mainV2.c`)**: Uses parallel file I/O with `MPI_File_read_at`
  - Supports `pack` and `scatter` modes with optional `:excl` binding

### Hybrid MPI+OpenMP Versions
- **Hybrid V1 (`hybridV1.c`)**: Thread-private CMS copies (best hybrid performance)
- **Hybrid V2 (`hybridV2.c`)**: Shared CMS with atomic operations (poor scaling)
- **Hybrid V3 (`hybridV3.c`)**: Alternative hybrid implementation

### OpenMP Versions
- **OpenMP V1 (`openmp_only.c`)**: Thread-private CMS copies
- **OpenMP V2 (`openmp_only2.c`)**: Shared CMS with atomic operations

### Serial Version
- **Serial (`cms_linear.c`)**: Baseline sequential implementation

## Project Structure

```
Parallel-Count-Min-Sketch/
├── count_min_sketch.h/c                                                    # Core CMS data structure
├── cms_linear.c, cms_linear_with_accuracy.c                                # Serial implementation
├── main.c, mainV2.c                                                        # MPI implementations
├── hybridV1.c, hybridV2.c, hybridV3.c                                      # Hybrid MPI+OpenMP
├── openmp_only.c, openmp_only2.c                                           # OpenMP implementations
├── only_cms_linear.sh, only_mainV*.sh, only_hybridV*.sh, only_openmp*.sh   # Execution scripts
├── Makefile                                                                # Build configuration
├── scripts/                                                                # Data generation and analysis
├── csv_results/                                                            # Benchmark results
├── plots/                                                                  # Generated visualizations
└── data/                                                                   # Test datasets
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
mpicc -g -Wall -std=c99 -o mainV2 count_min_sketch.c mainV2.c -lm
```

**Hybrid Version:**
```bash
mpicc -g -Wall -std=c99 -fopenmp -o hybridV1 count_min_sketch_hybridV1.c hybridV1.c -lm
```

**OpenMP Version:**
```bash
gcc -g -Wall -std=c99 -fopenmp -o openmp_only count_min_sketch.c openmp_only.c -lm
```

Or use the provided Makefile:
```bash
make
```

## Running

### MPI Execution
```bash
mpirun -np 4 ./mainV2 data/dataset_250m.txt
```

Or use the provided script:
```bash
./only_mainV2.sh
```

### Hybrid Execution
```bash
mpirun -np 2 ./hybridV1 data/dataset_250m.txt 4
# 2 MPI processes × 4 OpenMP threads = 8 total cores
```

Or use the provided scripts:
```bash
./only_hybridV1.sh
./only_hybridV2.sh
./only_hybridV3.sh
```

### OpenMP Execution
```bash
export OMP_NUM_THREADS=8
./openmp_only data/dataset_250m.txt
```

Or use the provided scripts:
```bash
./only_openmp.sh    # OpenMP V1 (thread-private CMS)
./only_openmp2.sh   # OpenMP V2 (shared CMS with atomics)
```

### Cluster Submission (PBS)
For cluster execution with job scheduler:
```bash
qsub only_mainV2.sh
qsub only_hybridV1.sh
qsub only_openmp.sh
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
```bash
python benchmark_metrics.py      # Compute speedup and efficiency
```
