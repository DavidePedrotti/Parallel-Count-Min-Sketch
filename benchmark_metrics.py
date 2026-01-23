"""
Benchmark script to calculate Speedup, Efficiency and Scalability
for Count-Min Sketch MPI implementations
"""

import subprocess
import re
import shutil
import csv
import os
from datetime import datetime

DATASET = "scripts/dataset_1000000000.txt"
FOLDER = "scripts/"

MPIRUN = "mpirun.actual" if shutil.which("mpirun.actual") else "mpirun"

def run_command(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout + result.stderr

def extract_time(output, pattern):
    """Extract time from output using regex"""
    match = re.search(pattern, output)
    if match:
        return float(match.group(1))
    return None

def extract_query_times(output):
    """Extract point, range, and inner product query times"""
    point_time = extract_time(output, r"Point query time:\s+([\d.]+) s")
    range_time = extract_time(output, r"Range query time:\s+([\d.]+) s")
    inner_time = extract_time(output, r"Inner product time:\s+([\d.]+) s")
    return point_time, range_time, inner_time

print(" COUNT-MIN SKETCH PERFORMANCE ANALYSIS")

# Run sequential baseline
print("Running sequential baseline")
cmd = f"{MPIRUN} -np 1 ./cms_linear {DATASET} {FOLDER}"
output = run_command(cmd)
print("DEBUG OUTPUT:", output)
T1 = extract_time(output, r"Total time taken to update CMS: ([\d.]+) seconds")
print(f"PROVA T1: {T1}")

if T1 is None:
    print("ERROR: Could not extract sequential time")
    exit(1)

# Extract query times for baseline
baseline_point, baseline_range, baseline_inner = extract_query_times(output)

print(f"Sequential time (T1): {T1:.3f}s")
if baseline_point is not None:
    print(f"  Point query: {baseline_point:.6f}s")
    print(f"  Range query: {baseline_range:.6f}s")
    print(f"  Inner product: {baseline_inner:.6f}s")
print()

# Test different process counts
processes = [1, 2, 4, 8]
results_main = []
results_mainv2 = []
results_mainv3 = []

print("MAIN.C (MPI Basic Implementation)")

for P in processes:
    print(f"Running main.c with {P} processes")
    cmd = f"{MPIRUN} -np {P} ./main {DATASET}"
    output = run_command(cmd)
    T = extract_time(output, r"Total time \(all elements, V1 structure\) = ([\d.]+) seconds")
    if T is None:
        T = extract_time(output, r"Total time = ([\d.]+) seconds")
    
    if T is None:
        print(f"  ERROR: Could not extract time for {P} processes")
        continue
    
    # Extract query times
    point_time, range_time, inner_time = extract_query_times(output)
    
    results_main.append({
        'processes': P,
        'time': T,
        'speedup': 0,  # Calculated later
        'efficiency': 0,
        'point_query': point_time,
        'range_query': range_time,
        'inner_product': inner_time
    })
    
    print(f"Time with {P} processes: {T:.3f}s")

# Calculate speedup/efficiency for main.c using its own 1-process time
if len(results_main) > 0:
    T_serial_main = results_main[0]['time']
    for r in results_main:
        r['speedup'] = T_serial_main / r['time'] if r['time'] > 0 else 0
        r['efficiency'] = r['speedup'] / r['processes'] if r['processes'] > 0 else 0

print("MAINV2.C (MPI-I/O Implementation)")
 
for P in processes:
    print(f"Running mainV2.c with {P} processes")
    cmd = f"{MPIRUN} -np {P} ./mainV2 {DATASET}"
    output = run_command(cmd)
    T = extract_time(output, r"Total time V2 full chunk: ([\d.]+) seconds")
    
    if T is None:
        T = extract_time(output, r"Total time:[\s]+([\d.]+) s")
        if T is None:
            T = extract_time(output, r"Total time taken to update CMS: ([\d.]+) seconds")
        if T is None:
            print(f"  ERROR: Could not extract time for {P} processes")
            print(f"  Last 400 chars of output:\n{output[-400:]}")
            continue
    
    # Extract query times
    point_time, range_time, inner_time = extract_query_times(output)
    
    results_mainv2.append({
        'processes': P,
        'time': T,
        'speedup': 0,  # Calculated later
        'efficiency': 0,
        'point_query': point_time,
        'range_query': range_time,
        'inner_product': inner_time
    })
    
    print(f"Time with {P} processes: {T:.3f}s")

# Calculate speedup/efficiency for mainV2.c using its own 1-process time
if len(results_mainv2) > 0:
    T_serial_mainv2 = results_mainv2[0]['time']
    for r in results_mainv2:
        r['speedup'] = T_serial_mainv2 / r['time'] if r['time'] > 0 else 0
        r['efficiency'] = r['speedup'] / r['processes'] if r['processes'] > 0 else 0

print("MAINV3.C (MPI-I/O Optimized Implementation)")

for P in processes:
    print(f"Running mainV3.c with {P} processes")
    cmd = f"{MPIRUN} -np {P} ./mainV3 {DATASET}"
    output = run_command(cmd)
    T = extract_time(output, r"Total time V3: ([\d.]+) seconds")
    
    if T is None:
        print(f"  ERROR: Could not extract time for {P} processes")
        continue
    
    # Extract query times
    point_time, range_time, inner_time = extract_query_times(output)
    
    results_mainv3.append({
        'processes': P,
        'time': T,
        'speedup': 0,  # Calculated later
        'efficiency': 0,
        'point_query': point_time,
        'range_query': range_time,
        'inner_product': inner_time
    })
    
    print(f"Time with {P} processes: {T:.3f}s")

# Calculate speedup/efficiency for mainV3.c using its own 1-process time
if len(results_mainv3) > 0:
    T_serial_mainv3 = results_mainv3[0]['time']
    for r in results_mainv3:
        r['speedup'] = T_serial_mainv3 / r['time'] if r['time'] > 0 else 0
        r['efficiency'] = r['speedup'] / r['processes'] if r['processes'] > 0 else 0

print("   PERFORMANCE METRICS - MAIN.C")

# Print summary table for main.c
print("│ Processes │ Time (s) │ Speedup  │ Efficiency │")

for r in results_main:
    print(f"│     {r['processes']}     │  {r['time']:6.3f}  │  {r['speedup']:5.2f}x  │   {r['efficiency']*100:5.1f}%   │")

print("   PERFORMANCE METRICS - MAINV2.C")

# Print summary table for mainV2.c
print("│ Processes │ Time (s) │ Speedup  │ Efficiency │")

for r in results_mainv2:
    print(f"│     {r['processes']}     │  {r['time']:6.3f}  │  {r['speedup']:5.2f}x  │   {r['efficiency']*100:5.1f}%   │")

print("   PERFORMANCE METRICS - MAINV3.C")

# Print summary table for mainV3.c
print("│ Processes │ Time (s) │ Speedup  │ Efficiency │")

for r in results_mainv3:
    print(f"│     {r['processes']}     │  {r['time']:6.3f}  │  {r['speedup']:5.2f}x  │   {r['efficiency']*100:5.1f}%   │")


# Comparative analysis
print("   COMPARATIVE ANALYSIS")

print("│ Processes │ Baseline │  main.c  │ mainV2.c │ mainV3.c │")
for i, P in enumerate(processes):
    if i < len(results_main) and i < len(results_mainv2) and i < len(results_mainv3):
        time_main = results_main[i]['time']
        time_mainv2 = results_mainv2[i]['time']
        time_mainv3 = results_mainv3[i]['time']
        baseline_val = f"{T1:6.3f}s" if P == 1 else "   -    "
        print(f"│     {P}     │ {baseline_val} │ {time_main:6.3f}s │ {time_mainv2:6.3f}s │ {time_mainv3:6.3f}s │")

# Save results to CSV for plotting
csv_filename = "benchmark_results.csv"
dataset_name = os.path.basename(DATASET)
dataset_size_mb = os.path.getsize(DATASET) / (1024*1024)

file_exists = os.path.exists(csv_filename)

with open(csv_filename, 'a', newline='') as csvfile:
    fieldnames = ['dataset', 'dataset_size_mb', 'processes', 'baseline', 'main', 'mainV2', 'mainV3']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    
    if not file_exists:
        writer.writeheader()
    
    for i, P in enumerate(processes):
        if i < len(results_main) and i < len(results_mainv2) and i < len(results_mainv3):
            writer.writerow({
                'dataset': dataset_name,
                'dataset_size_mb': f"{dataset_size_mb:.1f}",
                'processes': P,
                'baseline': T1 if P == 1 else '',
                'main': results_main[i]['time'],
                'mainV2': results_mainv2[i]['time'],
                'mainV3': results_mainv3[i]['time']
            })

print(f"\n✓ Results saved to {csv_filename}")


# Scalability analysis
print("SCALABILITY ANALYSIS")

print("\nSTRONG SCALABILITY - MAIN.C")
if len(results_main) >= 2:
    eff_values = [r['efficiency'] for r in results_main]
    eff_variation = max(eff_values) - min(eff_values)
    
    for r in results_main:
        print(f"  {r['processes']} cores: {r['efficiency']*100:.1f}%")
    
    print(f"Variation: {eff_variation*100:.1f}pp")

print("\nSTRONG SCALABILITY - MAINV2.C")
if len(results_mainv2) >= 2:
    eff_values = [r['efficiency'] for r in results_mainv2]
    eff_variation = max(eff_values) - min(eff_values)
    
    for r in results_mainv2:
        print(f"  {r['processes']} cores: {r['efficiency']*100:.1f}%")
    
    print(f"Variation: {eff_variation*100:.1f}pp")

print("\nSTRONG SCALABILITY - MAINV3.C")
if len(results_mainv3) >= 2:
    eff_values = [r['efficiency'] for r in results_mainv3]
    eff_variation = max(eff_values) - min(eff_values)
    
    for r in results_mainv3:
        print(f"  {r['processes']} cores: {r['efficiency']*100:.1f}%")
    
    print(f"Variation: {eff_variation*100:.1f}pp")
