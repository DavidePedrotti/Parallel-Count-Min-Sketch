#!/bin/bash
#PBS -N cms_benchmark
#PBS -l select=1:ncpus=8:mem=4gb
#PBS -l walltime=0:30:00
#PBS -q short_HPC4DS

mkdir -p output error

cd $PBS_O_WORKDIR

module load mpich-3.2

# Run benchmark
mpirun.actual -np 2 ./mainV2 scripts/dataset_10000_ordered.txt