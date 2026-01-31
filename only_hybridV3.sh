#!/bin/bash
#PBS -N cms_benchmark
#PBS -l select=1:ncpus=8:mem=2gb
#PBS -l place=pack:excl
#PBS -l walltime=0:20:00
#PBS -q short_cpuQ

cd $PBS_O_WORKDIR
module load mpich-3.2
export OMP_NUM_THREADS=4

# Array dei dataset
datasets=(
    data/dataset_10000_ordered.txt
    data/dataset_1000000_ordered.txt
    data/dataset_10000000_ordered.txt
    data/dataset_100000000_ordered.txt
    data/dataset_1000000000_ordered.txt
)

# File unico per l'output
output_file="output_V3.txt"

# Loop sui dataset
for dataset in "${datasets[@]}"; do
    echo "Running hybridV3 on $dataset ..." >> "$output_file"
    mpirun.actual -n 2 ./hybridV3 "$dataset" >> "$output_file" 2>&1
done
