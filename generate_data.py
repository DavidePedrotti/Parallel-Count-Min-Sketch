import random

num_procs = 4 
items_per_proc = 1000
total_items = num_procs * items_per_proc 

filename = "dataset.txt"

print(f"Generation of {total_items} integers in the file '{filename}'")

with open(filename, "w") as f:
    for _ in range(total_items):
        r = random.random()
        
        if r < 0.10:
            val = 123       # 10% of probability that it is 123 (Target A)
        elif r < 0.20:
            val = 456       # 10% of probability that it is 456 (Target B)
        elif r < 0.30:
            val = random.randint(100, 110) # 10% probability range [100-110]
        else:
            val = random.randint(1000, 9999) # "Random noise"
            
        f.write(f"{val}\n")

