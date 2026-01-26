#!/bin/bash
#PBS -N frequency_counter          
#PBS -l select=1:ncpus=8:mem=4gb
#PBS -l walltime=02:00:00
#PBS -q short_HPC4DS

cd $PBS_O_WORKDIR

if command -v python3 &> /dev/null; then
    PYTHON_CMD=python3
elif command -v python3.9 &> /dev/null; then
    PYTHON_CMD=python3.9
elif command -v python3.8 &> /dev/null; then
    PYTHON_CMD=python3.8
elif command -v python3.7 &> /dev/null; then
    PYTHON_CMD=python3.7
else
    PYTHON_CMD=python
fi

$PYTHON_CMD ./scripts/frequency_counter.py 
