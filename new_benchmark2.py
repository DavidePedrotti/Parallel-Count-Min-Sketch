import subprocess
import re
import csv
import os
import time

test_configurations = [
    {"chunks": 1, "cores_per_chunk": 1, "mode": "pack", "processes": 1},
    {"chunks": 1, "cores_per_chunk": 2, "mode": "pack", "processes": 2},
    {"chunks": 2, "cores_per_chunk": 1, "mode": "pack", "processes": 2},
    {"chunks": 1, "cores_per_chunk": 4, "mode": "pack", "processes": 4},
    {"chunks": 4, "cores_per_chunk": 1, "mode": "pack", "processes": 4},
    {"chunks": 2, "cores_per_chunk": 4, "mode": "pack", "processes": 8},
]

RESULTS_DIR = "output_results"
os.makedirs(RESULTS_DIR, exist_ok=True)

def extract_number_from_dataset(dataset_name):
    m = re.search(r"dataset_(\d+)_sorted\.txt", dataset_name)
    return m.group(1) if m else "unknown"

def gen_pbs_script(chunks, cores, mode, processes, exe, data_dir, dataset):
    name = f"{extract_number_from_dataset(dataset)}_{chunks}_{processes}"
    job_dir = f"{RESULTS_DIR}/{name}"
    os.makedirs(job_dir, exist_ok=True)
    walltime = "0:30:00"
    queue = "short_HPC4DS"

    return f"""#!/bin/bash
#PBS -N {name}
#PBS -l select={chunks}:ncpus={cores}:mem=4gb
#PBS -l walltime={walltime}
#PBS -q {queue}

cd $PBS_O_WORKDIR
module load mpich-3.2

mpirun.actual -np {processes} ./{exe} {data_dir}/{dataset} {data_dir}/ \
  > {job_dir}/output.txt 2> {job_dir}/error.txt
"""

def extract_time(output, pattern):
    m = re.search(pattern, output)
    if m:
        return float(m.group(1))
    return None

def extract_query_times(output):
    point = extract_time(output, r"Point query time:\s+([\d.eE+-]+)\s*s")
    rng = extract_time(output, r"Range query time:\s+([\d.eE+-]+)\s*s")
    inner = extract_time(output, r"Inner product time:\s+([\d.eE+-]+)\s*s")
    return point, rng, inner

def run_benchmark(cfg, exe, data_dir, dataset):
    chunks = cfg['chunks']
    cores = cfg['cores_per_chunk']
    processes = cfg['processes']
    mode = cfg['mode']
    name = f"{extract_number_from_dataset(dataset)}_{chunks}_{processes}"
    job_dir = f"{RESULTS_DIR}/{name}"
    os.makedirs(job_dir, exist_ok=True)

    print(f"Running with {name} configuration")

    script_path = f"{RESULTS_DIR}/{name}.sh"
    with open(script_path, "w") as f:
        f.write(gen_pbs_script(chunks, cores, mode, processes, exe, data_dir, dataset))

    result = subprocess.run(["qsub", script_path], capture_output=True, text=True)
    if result.returncode != 0:
        print("qsub failed:", result.stderr)
        return None

    job_id = result.stdout.strip().split(".")[0]
    print(f"Job submitted: {job_id}")

    while True:
        status = subprocess.run(["qstat", job_id], capture_output=True, text=True)
        if job_id not in status.stdout:
            break
        print("Job still running...")
        time.sleep(5)
    print("Job completed")

    output_file = f"{job_dir}/output.txt"
    for _ in range(30):
        if os.path.exists(output_file):
            with open(output_file) as f:
                content = f.read()
                if "Total time V2 full chunk" in content:
                    output = content
                    break
        time.sleep(1)
    else:
        print("ERROR: output file not ready or incomplete")
        return None

    time_taken = extract_time(output, r"Total time V2 full chunk:\s+([\d.eE+-]+)\s+seconds")
    point, rng, inner = extract_query_times(output)

    return {
        "configuration": name,
        "mode": mode,
        "time_taken": time_taken,
        "point_query": point,
        "range_query": rng,
        "inner_product": inner
    }

def save_results(results, dataset):
    csv_file = f"{RESULTS_DIR}/benchmark_{extract_number_from_dataset(dataset)}.csv"
    write_header = not os.path.exists(csv_file)

    with open(csv_file, "a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "configuration", "mode", "time_taken", "point_query", "range_query", "inner_product"
        ])
        if write_header:
            writer.writeheader()
        for r in results:
            writer.writerow(r)
    print(f"Results saved to {csv_file}")

def main():
    exe = "mainV2"
    data_dir = "data"
    dataset = "dataset_5000000_sorted.txt"

    all_results = []

    for cfg in test_configurations:
        print("\nStarting benchmark:", cfg)
        res = run_benchmark(cfg, exe, data_dir, dataset)
        if res:
            print(res)
            all_results.append(res)

    save_results(all_results, dataset)
    print("\nAll benchmarks completed")

if __name__ == "__main__":
    main()
