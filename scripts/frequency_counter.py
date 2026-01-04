def count_frequencies(filename):
  count_dict = defaultdict()
  with open(filename, "r") as f:
    for line in f:
      n = int(line.strip())
      count_dict[n] += 1
  with open("total_" + filename, "w+") as f:
    for key, val in count_dict.items():
      f.write(f"{key} {val}\n")

if __name__ == "__main__":
  count_frequencies("dataset_50000_ordered.txt")