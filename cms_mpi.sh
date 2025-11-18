#!/bin/bash
#PBS -N cms_mpi
#PBS -l select=1:ncpus=4
#PBS -l walltime=00:10:00
#PBS -q short_cpuQ
#PBS -o output/
#PBS -e error/

mkdir -p output error

cd $PBS_O_WORKDIR

module load mpich-3.2

mpicc -std=c99 -g -Wall -o cms_mpi main.c count_min_sketch.c -lm

NP=4

mpirun.actual -n $NP ./cms_mpi
