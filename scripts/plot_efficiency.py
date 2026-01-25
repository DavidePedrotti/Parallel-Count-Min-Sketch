import pandas as pd
import matplotlib.pyplot as plt
import os
import ast

def parse_configuration(config_str):
    try:
        return ast.literal_eval(config_str)
    except:
        return {}

def analyze_csv(csv_path):
    data = pd.read_csv(csv_path)

    data['config'] = data['configuration'].apply(parse_configuration)
    data['processes'] = data['config'].apply(lambda x: x.get('processes', 0))
    data['chunks'] = data['config'].apply(lambda x: x.get('chunks', 0))
    data['cores_per_chunk'] = data['config'].apply(lambda x: x.get('cores_per_chunk', 0))

    data['config_label'] = data.apply(
        lambda x: f"{x['chunks']}x{x['cores_per_chunk']}", axis=1
    )

    data = data.sort_values('processes')

    data = data.loc[data.groupby('processes')['avg_time_taken'].idxmin()]

    baseline = data[data['processes'] == 1]['avg_time_taken'].iloc[0]

    data['speedup'] = baseline / data['avg_time_taken']
    data['efficiency'] = (data['speedup'] / data['processes']) * 100

    return data


def plot_efficiency(csv_files, folder='data', output='efficiency_plot.png'):
    plt.figure(figsize=(10, 6))

    for csv_info in csv_files:
        csv_path = os.path.join(folder, csv_info['path'])

        if not os.path.exists(csv_path):
            print(f"Warning: {csv_path} not found, skipping...")
            continue

        df = analyze_csv(csv_path)

        label = csv_info['label']

        plt.plot(df['processes'], df['efficiency'], 'o-',
                label=label, linewidth=2, markersize=8)

        print(f"  Processes: {df['processes'].tolist()}")
        print(f"  Best configs: {df['config_label'].tolist()}")
        print(f"  Times: {df['avg_time_taken'].round(2).tolist()}s")
        print(f"  Efficiency: {df['efficiency'].round(1).tolist()}%")

    plt.axhline(y=100, color='k', linestyle='--', alpha=0.5,
                label='Ideal (100%)', linewidth=2)

    plt.xlabel('Number of Processes', fontsize=12)
    plt.ylabel('Efficiency (%)', fontsize=12)
    plt.title('Efficiency v2 500M Elements',
              fontsize=14, fontweight='bold')
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    plt.ylim(0, 110)

    plt.tight_layout()
    plt.savefig(output, dpi=300, bbox_inches='tight')
    print(f"\nPlot saved to: {output}")
    plt.show()

if __name__ == "__main__":
    csv_files = [
        {'path': 'benchmark_results_500000000_v2_pack.csv', 'label': 'Pack'},
        {'path': 'benchmark_results_500000000_v2_packexcl.csv', 'label': 'PackExcl'},
        {'path': 'benchmark_results_500000000_v2_scatter.csv', 'label': 'Scatter'},
        {'path': 'benchmark_results_500000000_v2_scatterexcl.csv', 'label': 'ScatterExcl'},
    ]

    folder = '../csv_results/'

    plot_efficiency(csv_files, folder=folder, output='efficiency_plot.png')