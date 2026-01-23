import pandas as pd
import matplotlib.pyplot as plt


import pandas as pd
import matplotlib.pyplot as plt

# dati dal CSV esportato dal benchmark
csv_filename = "benchmark_results.csv"
df = pd.read_csv(csv_filename)

# colonna da confrontare ('main', 'mainV2', 'mainV3')
colonna = 'mainV2'  

df = df[df[colonna].notnull()]
df['processes'] = df['processes'].astype(int)

datasets = df['dataset'].unique()
plt.figure(figsize=(10, 7))

for dataset in datasets:
    dati = df[df['dataset'] == dataset].sort_values('processes')
    if dati.empty:
        continue
    # tempo seriale (processes==1) per questo dataset
    serial_row = dati[dati['processes'] == 1]
    if serial_row.empty:
        continue
    tempo_seriale = serial_row[colonna].values[0]
    speedup = tempo_seriale / dati[colonna].astype(float)
    plt.plot(dati['processes'], speedup, marker='o', label=dataset)

plt.xlabel('N of parallel process', fontsize=14)
plt.ylabel(r'Time$_{serial}$ / Time$_{parallel}$', fontsize=16)
plt.legend(title="Dataset", fontsize=12)
plt.grid(True, which='both', linestyle='--', alpha=0.5)
plt.tight_layout()
plt.savefig("speedup_plot.png", dpi=200)
plt.show()
