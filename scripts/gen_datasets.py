import random
import argparse

def gen_distributed(filename, n, seed=12345):
    random.seed(seed)
    with open(filename, "w") as f:
        for _ in range(n):
            r = random.random()

            if r < 0.10:
                val = 123
            elif r < 0.20:
                val = 456
            elif r < 0.30:
                val = random.randint(100, 110)
            else:
                val = random.randint(1000, 9999)

            f.write(f"{val}\n")

def sort_file(infile, outfile):
    with open(infile, "r") as f:
        nums = [int(x.strip()) for x in f if x.strip()]
    nums.sort()
    with open(outfile, "w") as f:
        for x in nums:
            f.write(f"{x}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--sizes", nargs="+", type=int, default=[10000, 1000000, 10000000, 10000000, 100000000, 1000000000])
    parser.add_argument("--seed", type=int, default=12345)
    args = parser.parse_args()

    for n in args.sizes:
        fn = f"dataset_{n}.txt"
        print(f"\n### Generating {fn} ({n} elements) ###")
        
        gen_distributed(fn, n, seed=args.seed)
        
        out_sorted = fn.replace(".txt", "_ordered.txt")
        print(f"Sorting -> {out_sorted}")
        sort_file(fn, out_sorted)

    print("\nDone.")
