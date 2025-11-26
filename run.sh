#!/bin/bash
#PBS -l select=2:ncpus=1:mem=2gb -l place=pack
#PBS -o output_$1.txt
#PBS -e error_$1.txt

#PBS -l walltime=0:05:00

#PBS -q short_cpuQ

module load mpich-3.2
mpirun.actual -n 2 ./project/main ./project/data/dataset_10000000.txt