import os

def count_frequencies(filename, dir=""):
    count_dict = {}

    # Input path: se dir è vuoto, filename può essere assoluto o relativo
    input_path = os.path.join(dir, filename) if dir else filename

    # Leggi file e conta frequenze
    with open(input_path, "r") as f:
        for line in f:
            n = int(line.strip())
            count_dict[n] = count_dict.get(n, 0) + 1

    # Output path: scrivi nella stessa cartella del file
    # Usa os.path.basename per prendere solo il nome del file
    filename_only = os.path.basename(filename)
    output_dir = dir if dir else os.path.dirname(filename)
    output_path = os.path.join(output_dir, "total_" + filename_only)

    with open(output_path, "w+") as f:
        for key, val in count_dict.items():
            f.write(f"{key} {val}\n")


if __name__ == "__main__":
    # Esempio con percorso assoluto
    count_frequencies("/home/giorgia.gabardi/Parallel-Count-Min-Sketch/data/dataset_10000_ordered.txt")
