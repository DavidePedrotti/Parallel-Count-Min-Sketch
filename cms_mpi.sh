#!/bin/bash
#PBS -N cms_mpi
#PBS -l select=1:ncpus=4
#PBS -l walltime=00:10:00
#PBS -q short_cpuQ

mkdir -p output error

cd $PBS_O_WORKDIR

module load mpich-3.2

NP=4

mpirun.actual -n $NP ./cms_mpi
