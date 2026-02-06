#!/bin/bash
#PBS -N cms_omp_benchmark
#PBS -l select=1:ncpus=8:mem=2gb
#PBS -l place=pack:excl
#PBS -l walltime=0:20:00
#PBS -q short_cpuQ

cd $PBS_O_WORKDIR

# module load gcc

export OMP_NUM_THREADS=4
export OMP_PROC_BIND=close
export OMP_PLACES=cores

datasets=(
    data/dataset_10000_ordered.txt
    data/dataset_1000000_ordered.txt
    data/dataset_10000000_ordered.txt
    data/dataset_100000000_ordered.txt
    data/dataset_1000000000_ordered.txt
)

output_file="output_omp2.txt"

for dataset in "${datasets[@]}"; do
    echo "Running openmp_only2 on $dataset ..." >> "$output_file"
    ./openmp_only2 "$dataset" >> "$output_file" 2>&1
done
