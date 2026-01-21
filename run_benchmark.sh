#!/bin/bash
#PBS -N cms_benchmark
#PBS -l select=1:ncpus=8:mem=4gb
#PBS -l walltime=0:30:00
#PBS -q short_HPC4DS

mkdir -p output error

cd $PBS_O_WORKDIR

module load mpich-3.2

# Try to find Python 3
if command -v python3 &> /dev/null; then
    PYTHON_CMD=python3
elif command -v python3.9 &> /dev/null; then
    PYTHON_CMD=python3.9
elif command -v python3.8 &> /dev/null; then
    PYTHON_CMD=python3.8
elif command -v python3.7 &> /dev/null; then
    PYTHON_CMD=python3.7
else
    PYTHON_CMD=python
fi

echo "Using Python: $PYTHON_CMD"
$PYTHON_CMD --version

# Compile all versions
mpicc -g -Wall -std=c99 -o cms_linear count_min_sketch.c cms_linear.c -lm
mpicc -g -Wall -std=c99 -o main count_min_sketch.c main.c -lm
mpicc -g -Wall -std=c99 -o mainV2 count_min_sketch.c mainV2.c -lm
mpicc -g -Wall -std=c99 -o mainV3 count_min_sketch.c mainV3.c -lm

# Run benchmark
$PYTHON_CMD benchmark_metrics.py

