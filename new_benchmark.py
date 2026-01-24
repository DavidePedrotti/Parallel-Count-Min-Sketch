import subprocess
import re
import shutil
import csv
import os
from datetime import datetime
import time

test_configurations = [
  {"chunks" : 1, "cores_per_chunk" : 1, "mode" : "pack", "processes" : 1},
  {"chunks" : 1, "cores_per_chunk" : 2, "mode" : "pack", "processes" : 2},
  {"chunks" : 2, "cores_per_chunk" : 1, "mode" : "pack", "processes" : 2},
  {"chunks" : 1, "cores_per_chunk" : 4, "mode" : "pack", "processes" : 4},
  {"chunks" : 4, "cores_per_chunk" : 1, "mode" : "pack", "processes" : 4},
  {"chunks" : 2, "cores_per_chunk" : 4, "mode" : "pack", "processes" : 8},
]

RESULTS_DIR = "output_results"
os.makedirs(RESULTS_DIR, exist_ok=True)

def gen_pbs_script(chunks, cores_per_chunk, mode, processes, executable_name, dataset_folder, dataset_name):
  name = f"{extract_number_from_dataset(dataset_name)}_{chunks}_{processes}"
  walltime = "0:30:00"
  queue = "short_HPC4DS"
  pbs_script = f"""#!/bin/bash
#PBS -N {name}

#PBS -l select={chunks}:ncpus={cores_per_chunk}:mem=4gb
#PBS -l walltime={walltime}
#PBS -q {queue}

RESULTS_DIR="$PBS_O_WORKDIR/{RESULTS_DIR}"

mkdir -p $RESULTS_DIR/{name}/

cd $PBS_O_WORKDIR

module load mpich-3.2
module load python-3.10.14

mpirun.actual -np {processes} ./{executable_name} {dataset_folder}/{dataset_name} {dataset_folder}/ > {RESULTS_DIR}/{name}/output.txt 2> {RESULTS_DIR}/{name}/error.txt
"""
  return pbs_script

def extract_time(output, pattern):
  match = re.search(pattern, output)
  if match:
      return float(match.group(1))
  return None

def extract_query_times(output):
  point_time = extract_time(output, r"Point query time:\s+([\d.]+) s")
  range_time = extract_time(output, r"Range query time:\s+([\d.]+) s")
  inner_time = extract_time(output, r"Inner product time:\s+([\d.]+) s")
  return point_time, range_time, inner_time

def run_command(cmd):
  result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
  return result.stdout + result.stderr

def run_benchmark(configuration, executable_name, dataset_folder, dataset_name):
  chunks = configuration['chunks']
  cores_per_chunk = configuration['cores_per_chunk']
  mode = configuration['mode']
  processes = configuration['processes']
  
  name = f"{extract_number_from_dataset(dataset_name)}_{chunks}_{processes}"
  print(f"Running with {name} configuration")

  pbs_script = gen_pbs_script(chunks, cores_per_chunk, mode, processes, executable_name, dataset_folder, dataset_name)
  
  pbs_filename = f"{RESULTS_DIR}/{name}.sh"
  with open(pbs_filename, 'w') as f:
    f.write(pbs_script)

  print(f"Submitting job: {pbs_filename}")
  result = subprocess.run(["qsub", pbs_filename], capture_output=True, text=True)

  print(f"qsub stdout: {result.stdout}")
  print(f"qsub stderr: {result.stderr}")
  if result.returncode != 0:
    print("Job submission failed!")
    return

  output_file = f"{RESULTS_DIR}/{name}/output.txt"
  error_file = f"{RESULTS_DIR}/{name}/error.txt"

  print("Waiting for job to complete...")
  job_id = result.stdout.split(".")[0]
  while True:
    status = subprocess.run(["qstat", job_id], capture_output=True, text=True)
    
    status_out = status.stdout
    if "Job has finished" in str(status):
      print("Job completed successfully")
      break
    else:
      print("Job still running")
    time.sleep(5)

  with open(output_file, 'r') as f:
    output = f.read()
    if not output.strip():
      print("Error: output file is empty")
      return None

  time_taken = extract_time(output, r"Total time V2 full chunk: ([\d.]+) seconds") # for mainV2
  point_time, range_time, inner_time = extract_query_times(output)

  return {
      'configuration': name,
      'mode': mode,
      'time_taken': time_taken,
      'point_query': point_time,
      'range_query': range_time,
      'inner_product': inner_time
  }

def main():
  executable_name = "mainV2"
  dataset_folder = "data"
  dataset_name = "dataset_500000000_sorted.txt"

  all_results = []

  for config in test_configurations:
    print(f"\nStarting benchmark with configuration: {config}")
    result = run_benchmark(config, executable_name, dataset_folder, dataset_name)
    print(result)

    all_results.append(result)

    print(f"Configuration: {result['configuration']}")
    print(f"Mode: {result['mode']}")
    print(f"Time Taken: {result['time_taken']}s")
    print(f"Point Query Time: {result['point_query']}s")
    print(f"Range Query Time: {result['range_query']}s")
    print(f"Inner Product Time: {result['inner_product']}s")
    print("-" * 50)

  save_results_to_csv(all_results, dataset_name)

  # remove all output/error files
  subprocess.run(f"rm {extract_number_from_dataset(dataset_name)}_*", shell=True, capture_output=True, text=True)

def save_results_to_csv(all_results, dataset_name):
  csv_filename = f"{RESULTS_DIR}/benchmark_results_{extract_number_from_dataset(dataset_name)}.csv"
  file_exists = os.path.exists(csv_filename)

  with open(csv_filename, 'a', newline='') as csvfile:
    fieldnames = ['configuration', 'mode', 'time_taken', 'point_query', 'range_query', 'inner_product']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    if not file_exists:
      writer.writeheader()

    for result in all_results:
      writer.writerow({
        'configuration': result['configuration'],
        'mode': result['mode'],
        'time_taken': result['time_taken'],
        'point_query': result['point_query'],
        'range_query': result['range_query'],
        'inner_product': result['inner_product']
      })

  print(f"Results saved to {csv_filename}")

def extract_number_from_dataset(dataset_name):
  match = re.search(r"dataset_(\d+)_sorted\.txt", dataset_name)
  if match:
    return match.group(1)
  else:
    print(f"Error: failed to match name: {dataset_name}")
    return None

if __name__ == "__main__":
  main()