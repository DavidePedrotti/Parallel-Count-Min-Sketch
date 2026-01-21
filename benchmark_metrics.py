"""
Benchmark script to calculate Speedup, Efficiency, and Scalability
for Count-Min Sketch MPI implementations
"""

import subprocess
import re

DATASET = "data/dataset_5000000_sorted.txt"
FOLDER = "data/"

def run_command(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout + result.stderr

def extract_time(output, pattern):
    """Extract time from output using regex"""
    match = re.search(pattern, output)
    if match:
        return float(match.group(1))
    return None

print(" COUNT-MIN SKETCH PERFORMANCE ANALYSIS")
print(f"Dataset: 5M elements\n")

# Run sequential baseline
print("Running sequential baseline")
cmd = f"mpirun.actual -np 1 ./cms_linear {DATASET} {FOLDER}"
output = run_command(cmd)
print("Output:", output[:200])  # Debug: print first 200 chars
T1 = extract_time(output, r"Total time taken to update CMS: ([\d.]+) seconds")

if T1 is None:
    print("ERROR: Could not extract sequential time")
    exit(1)

print(f"Sequential time (T1): {T1:.3f}s\n")

# Test different process counts
processes = [2, 4, 8]
results_main = []
results_mainv2 = []
results_mainv3 = []

print("MAIN.C (MPI Basic Implementation)")

for P in processes:
    print(f"Running main.c with {P} processes")
    cmd = f"mpirun.actual -np {P} ./main {DATASET}"
    output = run_command(cmd)
    T = extract_time(output, r"Total time \(all elements, V1 structure\) = ([\d.]+) seconds")
    
    if T is None:
        print(f"  ERROR: Could not extract time for {P} processes")
        continue
    
    # Calculate metrics
    speedup = T1 / T if T > 0 else 0
    efficiency = speedup / P if P > 0 else 0
    
    results_main.append({
        'processes': P,
        'time': T,
        'speedup': speedup,
        'efficiency': efficiency
    })
    
    print(f"Time with {P} processes: {T:.3f}s")

print("MAINV2.C (MPI-I/O Implementation)")
 
for P in processes:
    print(f"Running mainV2.c with {P} processes")
    cmd = f"mpirun.actual -np {P} ./mainV2 {DATASET}"
    output = run_command(cmd)
    T = extract_time(output, r"Total time V2 full chunk: ([\d.]+) seconds")
    
    if T is None:
        # Try alternative pattern
        T = extract_time(output, r"Total time:[\s]+([\d.]+) s")
        if T is None:
            T = extract_time(output, r"Total time taken to update CMS: ([\d.]+) seconds")
        if T is None:
            print(f"  ERROR: Could not extract time for {P} processes")
            print(f"  Last 400 chars of output:\n{output[-400:]}")
            continue
    
    # Calculate metrics
    speedup = T1 / T if T > 0 else 0
    efficiency = speedup / P if P > 0 else 0
    
    results_mainv2.append({
        'processes': P,
        'time': T,
        'speedup': speedup,
        'efficiency': efficiency
    })
    
    print(f"Time with {P} processes: {T:.3f}s")

print("MAINV3.C (MPI-I/O Optimized Implementation)")

for P in processes:
    print(f"Running mainV3.c with {P} processes")
    cmd = f"mpirun.actual -np {P} ./mainV3 {DATASET}"
    output = run_command(cmd)
    T = extract_time(output, r"Total time V3: ([\d.]+) seconds")
    
    if T is None:
        print(f"  ERROR: Could not extract time for {P} processes")
        continue
    
    # Calculate metrics
    speedup = T1 / T if T > 0 else 0
    efficiency = speedup / P if P > 0 else 0
    
    results_mainv3.append({
        'processes': P,
        'time': T,
        'speedup': speedup,
        'efficiency': efficiency
    })
    
    print(f"Time with {P} processes: {T:.3f}s")

print("   PERFORMANCE METRICS - MAIN.C")

# Print summary table for main.c
print("│ Processes │ Time (s) │ Speedup  │ Efficiency │")
print(f"│     1     │  {T1:6.3f}  │  {1.00:5.2f}x  │   {100.0:5.1f}%   │")

for r in results_main:
    print(f"│     {r['processes']}     │  {r['time']:6.3f}  │  {r['speedup']:5.2f}x  │   {r['efficiency']*100:5.1f}%   │")

print("   PERFORMANCE METRICS - MAINV2.C")

# Print summary table for mainV2.c
print("│ Processes │ Time (s) │ Speedup  │ Efficiency │")
print(f"│     1     │  {T1:6.3f}  │  {1.00:5.2f}x  │   {100.0:5.1f}%   │")

for r in results_mainv2:
    print(f"│     {r['processes']}     │  {r['time']:6.3f}  │  {r['speedup']:5.2f}x  │   {r['efficiency']*100:5.1f}%   │")

print("   PERFORMANCE METRICS - MAINV3.C")

# Print summary table for mainV3.c
print("│ Processes │ Time (s) │ Speedup  │ Efficiency │")
print(f"│     1     │  {T1:6.3f}  │  {1.00:5.2f}x  │   {100.0:5.1f}%   │")

for r in results_mainv3:
    print(f"│     {r['processes']}     │  {r['time']:6.3f}  │  {r['speedup']:5.2f}x  │   {r['efficiency']*100:5.1f}%   │")


# Comparative analysis
print("   COMPARATIVE ANALYSIS")

print("│ Processes │  main.c  │ mainV2.c │ mainV3.c │")
for i, P in enumerate(processes):
    if i < len(results_main) and i < len(results_mainv2) and i < len(results_mainv3):
        time_main = results_main[i]['time']
        time_mainv2 = results_mainv2[i]['time']
        time_mainv3 = results_mainv3[i]['time']
        print(f"│     {P}     │ {time_main:6.3f}s │ {time_mainv2:6.3f}s │ {time_mainv3:6.3f}s │")


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
   
