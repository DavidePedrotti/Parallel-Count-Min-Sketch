#!/bin/bash
#PBS -N cms_benchmark
#PBS -l select=1:ncpus=8:mem=4gb
#PBS -l walltime=0:30:00
#PBS -q short_HPC4DS

mkdir -p output error
module load mpich-3.2

cd $PBS_O_WORKDIR

# Run benchmark
mpirun.actual -np 2 ./mainV2 data/dataset_1000000_ordered.txt >> output_mainV2.txt