# sort_dataset.py
import sys

INPUT_FILE = "dataset.txt"
OUTPUT_FILE = "dataset_sorted.txt"

def main():
    try:
        with open(INPUT_FILE, "r") as f:
            numbers = [int(line.strip()) for line in f if line.strip()]
        
        numbers.sort()
        
        with open(OUTPUT_FILE, "w") as f:
            for num in numbers:
                f.write(f"{num}\n")
        
        print(f"Dataset ordinato salvato in '{OUTPUT_FILE}'. Totale righe: {len(numbers)}")
    
    except FileNotFoundError:
        print(f"Errore: il file '{INPUT_FILE}' non esiste.")
    except ValueError as e:
        print(f"Errore: il file contiene valori non numerici. Dettagli: {e}")

if __name__ == "__main__":
    main()
