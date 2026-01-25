import os

def count_frequencies(filename, dir):
  count_dict = {}
  with open(os.path.join(dir, filename), "r") as f:
    for line in f:
      n = int(line.strip())
      count_dict[n] = count_dict.get(n, 0) + 1
  with open(os.path.join(dir, "total_" + filename), "w+") as f:
    for key, val in count_dict.items():
      f.write(f"{key} {val}\n")

if __name__ == "__main__":
  count_frequencies("dataset_50000000_sorted.txt", "../data/")