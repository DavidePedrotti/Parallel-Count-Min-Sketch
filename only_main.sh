#!/bin/bash
#PBS -N cms_benchmark
#PBS -l select=1:ncpus=8:mem=4gb
#PBS -l walltime=0:30:00
#PBS -q short_HPC4DS

mkdir -p output error
module load mpich-3.2

cd $PBS_O_WORKDIR

# Array dei dataset
datasets=(
    data/dataset_10000_ordered.txt
    data/dataset_1000000_ordered.txt
    data/dataset_10000000_ordered.txt
    data/dataset_100000000_ordered.txt
    data/dataset_1000000000_ordered.txt
)

output_file="output_main.txt"

# Loop sui dataset
for dataset in "${datasets[@]}"; do
    echo "Running main on $dataset ..." >> "$output_file"
    mpirun.actual -np 2 ./main "$dataset" >> "$output_file" 2>&1
done
