import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import ast

def parse_configuration(config_str):
    try:
        return ast.literal_eval(config_str)
    except:
        return {}

def analyze_csv(csv_path, baseline_time):
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

    data['speedup'] = baseline_time / data['avg_time_taken']
    data['efficiency'] = (data['speedup'] / data['processes']) * 100

    return data


def plot_efficiency(csv_files, baseline_file, folder='data', output='efficiency_plot.png'):
    baseline_path = os.path.join(folder, baseline_file)
    if not os.path.exists(baseline_path):
        raise FileNotFoundError(f"Baseline file not found: {baseline_path}")

    baseline_data = pd.read_csv(baseline_path)
    baseline_time = baseline_data['avg_time_taken'].iloc[0]
    print(f"Using baseline time from {baseline_file}: {baseline_time:.2f}s\n")

    plt.figure(figsize=(10, 6))

    all_processes = []

    for csv_info in csv_files:
        csv_path = os.path.join(folder, csv_info['path'])

        if not os.path.exists(csv_path):
            print(f"Warning: {csv_path} not found, skipping...")
            continue

        df = analyze_csv(csv_path, baseline_time)

        label = csv_info['label']

        plt.plot(df['processes'], df['efficiency'], 'o-',
                label=label, linewidth=2, markersize=8)

        print(f"{label}:")
        print(f"  Processes: {df['processes'].tolist()}")
        print(f"  Best configs: {df['config_label'].tolist()}")
        print(f"  Times: {df['avg_time_taken'].round(2).tolist()}s")
        print(f"  Speedup: {df['speedup'].round(2).tolist()}x")
        print(f"  Efficiency: {df['efficiency'].round(2).tolist()}%\n")

        all_processes.extend(df['processes'].tolist())

    if all_processes:
        max_processes = max(all_processes)
        ideal_x = np.array([1, max_processes])
        plt.plot(ideal_x, [100, 100], 'k--', alpha=0.5,
                label='Ideal (100%)', linewidth=2)

    plt.xlabel('Number of Processes', fontsize=12)
    plt.ylabel('Efficiency (%)', fontsize=12)
    plt.title('Efficiency v2 250M Elements',
              fontsize=14, fontweight='bold')
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    plt.xlim(left=1)
    plt.ylim(bottom=0, top=110)

    plt.tight_layout()
    plt.savefig(output, dpi=300, bbox_inches='tight')
    print(f"\nPlot saved to: {output}")
    plt.show()


if __name__ == "__main__":
    csv_files = [
        {'path': 'benchmark_results_250m_v2_pack.csv', 'label': 'Pack'},
        {'path': 'benchmark_results_250m_v2_packexcl.csv', 'label': 'PackExcl'},
        {'path': 'benchmark_results_250m_v2_scatter.csv', 'label': 'Scatter'},
        {'path': 'benchmark_results_250m_v2_scatterexcl.csv', 'label': 'ScatterExcl'},
    ]

    baseline_file = 'benchmark_results_250m_linear.csv'
    folder = '../csv_results/'

    plot_efficiency(csv_files, baseline_file, folder=folder, output='efficiency_plot_250m_v2.png')