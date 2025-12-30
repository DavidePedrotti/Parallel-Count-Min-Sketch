import numpy as np
import random

dataset_sizes = [50000, 500000, 5000000]

def generate_data(num_elements, min_val=0, max_val=9999):
  alpha = 2.0
  pareto_dist = np.random.pareto(alpha, num_elements)

  values = np.clip(pareto_dist * (max_val - min_val), min_val, max_val).astype(int)

  return values

def create_dataset(num_elements):
  data = generate_data(num_elements)
  data_sorted = np.sort(data)
  filename = f"../data/dataset_{num_elements}_sorted.txt"
  with open(filename, "w") as f:
    for value in data_sorted:
      f.write(f"{value}\n")
  
  print(f"Dataset with {num_elements} elements has been created\n")

if __name__ == "__main__":
  for size in dataset_sizes:
    create_dataset(size)