#!/bin/bash
#PBS -N cms_linear
#PBS -l select=1:ncpus=1:mem=2gb
#PBS -l walltime=0:10:00
#PBS -q short_HPC4DS

cd $PBS_O_WORKDIR

./cms_linear scripts/dataset_10000.txt scripts/