import subprocess
import re
import shutil
import csv
import os
from datetime import datetime
import time

test_configurations = [
  {"chunks" : 1, "cores_per_chunk" : 16, "mode" : "pack:excl", "threads" : 16},
  {"chunks" : 1, "cores_per_chunk" : 8, "mode" : "pack:excl", "threads" : 8},
  {"chunks" : 1, "cores_per_chunk" : 4, "mode" : "pack:excl", "threads" : 4},
  {"chunks" : 1, "cores_per_chunk" : 2, "mode" : "pack:excl", "threads" : 2},
  {"chunks" : 1, "cores_per_chunk" : 1, "mode" : "pack:excl", "threads" : 1},
]

RESULTS_DIR = "output_results"
os.makedirs(RESULTS_DIR, exist_ok=True)

def gen_pbs_script(chunks, cores_per_chunk, mode, threads, executable_name, dataset_folder, dataset_name):
  name = f"{extract_number_from_dataset(dataset_name)}_{chunks}_{threads}_packexcl"
  walltime = "0:30:00"
  queue = "short_HPC4DS"
  pbs_script = f"""#!/bin/bash
#PBS -N {name}

#PBS -l select={chunks}:ncpus={cores_per_chunk}:mem=4gb
#PBS -l place={mode}
#PBS -l walltime={walltime}
#PBS -q {queue}

RESULTS_DIR="$PBS_O_WORKDIR/{RESULTS_DIR}"

mkdir -p $RESULTS_DIR/{name}/

cd $PBS_O_WORKDIR

module load python-3.10.14

export OMP_NUM_THREADS={threads}
l
./{executable_name} {dataset_folder}/{dataset_name} {dataset_folder}/ > {RESULTS_DIR}/{name}/output.txt 2> {RESULTS_DIR}/{name}/error.txt
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

def gen_script(configuration, executable_name, dataset_folder, dataset_name):
  chunks = configuration['chunks']
  cores_per_chunk = configuration['cores_per_chunk']
  mode = configuration['mode']
  threads = configuration['threads']

  name = f"{extract_number_from_dataset(dataset_name)}_{chunks}_{threads}_packexcl"
  print(f"Running with {name} configuration")

  pbs_script = gen_pbs_script(chunks, cores_per_chunk, mode, threads, executable_name, dataset_folder, dataset_name)

  pbs_filename = f"{RESULTS_DIR}/{name}.sh"
  with open(pbs_filename, 'w') as f:
    f.write(pbs_script)

  return pbs_filename, name, mode

def run_benchmark(pbs_filename, name, mode):
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

  time_taken = extract_time(output, r"Total time: ([\d.]+) seconds")
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
  executable_name = "openmp_only"
  dataset_folder = "data"
  dataset_name = "dataset_250000000_sorted.txt"

  all_results = []

  for config in test_configurations:
    print(f"\nStarting benchmark with configuration: {config}")

    times_taken = []
    point_query_times = []
    range_query_times = []
    inner_product_times = []

    pbs_filename, name, mode = gen_script(config, executable_name, dataset_folder, dataset_name)
    for _ in range(10):
      result = run_benchmark(pbs_filename, name, mode)
      print(result)

      times_taken.append(result['time_taken'])
      point_query_times.append(result['point_query'])
      range_query_times.append(result['range_query'])
      inner_product_times.append(result['inner_product'])

    valid_times_taken = [time for time in times_taken if time is not None]
    avg_time_taken = sum(valid_times_taken) / len(valid_times_taken) if times_taken else 0

    valid_point_query_times = [time for time in point_query_times if time is not None]
    avg_point_query_time = sum(valid_point_query_times) / len(valid_point_query_times) if valid_point_query_times else 0

    valid_range_query_times = [time for time in range_query_times if time is not None]
    avg_range_query_time = sum(valid_range_query_times) / len(valid_range_query_times) if valid_range_query_times else 0

    valid_inner_product_times = [time for time in inner_product_times if time is not None]
    avg_inner_product_time = sum(valid_inner_product_times) / len(valid_inner_product_times) if valid_inner_product_times else 0

    print(f"Configuration: {config}")
    print(f"Mode: {result['mode']}")
    print(f"Average Time Taken: {avg_time_taken:.4f}s")
    print(f"Average Point Query Time: {avg_point_query_time:.4f}s")
    print(f"Average Range Query Time: {avg_range_query_time:.4f}s")
    print(f"Average Inner Product Time: {avg_inner_product_time:.4f}s")
    print("-" * 50)

    all_results.append({
        'configuration': config,
        'mode': result['mode'],
        'avg_time_taken': avg_time_taken,
        'avg_point_query': avg_point_query_time,
        'avg_range_query': avg_range_query_time,
        'avg_inner_product': avg_inner_product_time,
    })

  save_results_to_csv(all_results, dataset_name)

  # remove all output/error files
  subprocess.run(f"rm {extract_number_from_dataset(dataset_name)}_*", shell=True, capture_output=True, text=True)

def save_results_to_csv(all_results, dataset_name):
  csv_filename = f"{RESULTS_DIR}/benchmark_results_{extract_number_from_dataset(dataset_name)}_ompv1_packexcl.csv"
  file_exists = os.path.exists(csv_filename)

  with open(csv_filename, 'a', newline='') as csvfile:
    fieldnames = ['configuration', 'mode', 'avg_time_taken', 'avg_point_query', 'avg_range_query', 'avg_inner_product']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    if not file_exists:
      writer.writeheader()

    for result in all_results:
      writer.writerow({
        'configuration': result['configuration'],
        'mode': result['mode'],
        'avg_time_taken': result['avg_time_taken'],
        'avg_point_query': result['avg_point_query'],
        'avg_range_query': result['avg_range_query'],
        'avg_inner_product': result['avg_inner_product']
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