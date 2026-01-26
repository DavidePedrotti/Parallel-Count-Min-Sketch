#!/bin/bash
#PBS -N cms_benchmark
#PBS -l select=1:ncpus=8:mem=2gb
#PBS -l place=pack:excl
#PBS -l walltime=0:20:00
#PBS -q short_cpuQ

cd $PBS_O_WORKDIR

module load mpich-3.2
export OMP_NUM_THREADS=4

mpirun.actual -n 2 ./hybridV2 data/dataset_1000000000_ordered.txt >> output_v2.txt
