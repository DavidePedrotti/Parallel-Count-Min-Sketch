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
    data['efficiency'] = (data['speedup'] / data['processes'])

    return data


def create_matrix_table(dataset_configs, folder='data', output='matrix_table.png'):
    all_processes = set()
    best_results = {}

    for dataset_config in dataset_configs:
        size = dataset_config['size']
        csv_files = dataset_config['csv_files']
        baseline_file = dataset_config['baseline_file']

        baseline_path = os.path.join(folder, baseline_file)
        if not os.path.exists(baseline_path):
            print(f"Warning: Baseline file not found: {baseline_path}, skipping...")
            continue

        baseline_data = pd.read_csv(baseline_path)
        baseline_time = baseline_data['avg_time_taken'].iloc[0]
        print(f"\n{size} Elements - Using baseline time: {baseline_time:.2f}s")

        size_results = {}

        for csv_info in csv_files:
            csv_path = os.path.join(folder, csv_info['path'])

            if not os.path.exists(csv_path):
                print(f"Warning: {csv_path} not found, skipping...")
                continue

            df = analyze_csv(csv_path, baseline_time)
            label = csv_info['label']

            for _, row in df.iterrows():
                proc = int(row['processes'])
                all_processes.add(proc)

                if proc not in size_results or row['speedup'] > size_results[proc]['speedup']:
                    size_results[proc] = {
                        'speedup': row['speedup'],
                        'efficiency': row['efficiency'],
                        'method': label,
                        'config': row['config_label']
                    }

        best_results[size] = size_results

    processes = sorted(all_processes)

    table_data = []
    row_labels = []

    for size in [cfg['size'] for cfg in dataset_configs]:
        if size not in best_results:
            continue

        speedup_row = []
        for proc in processes:
            if proc in best_results[size]:
                speedup_row.append(f"{best_results[size][proc]['speedup']:.2f}")
            else:
                speedup_row.append("-")
        table_data.append(speedup_row)
        row_labels.append(f"{size} S")

        efficiency_row = []
        for proc in processes:
            if proc in best_results[size]:
                efficiency_row.append(f"{best_results[size][proc]['efficiency']:.2f}")
            else:
                efficiency_row.append("-")
        table_data.append(efficiency_row)
        row_labels.append(f"{size} E")

        print(f"\n{size} best methods:")
        for proc in processes:
            if proc in best_results[size]:
                result = best_results[size][proc]
                print(f"  {proc:2d} processes: {result['method']:11s} ({result['config']}) - "
                      f"Speedup: {result['speedup']:5.2f}x, Efficiency: {result['efficiency']:5.1f}%")

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.axis('tight')
    ax.axis('off')

    col_labels = [str(p) for p in processes]

    table = ax.table(cellText=table_data,
                     rowLabels=row_labels,
                     colLabels=col_labels,
                     cellLoc='center',
                     loc='center')

    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1, 2.5)

    for i in range(len(col_labels)):
        if (0, i) in table._cells:
            table[(0, i)].set_facecolor('#40466e')
            table[(0, i)].set_text_props(weight='bold', color='white')

    for i in range(len(row_labels)):
        if (i+1, -1) in table._cells:
            table[(i+1, -1)].set_facecolor('#40466e')
            table[(i+1, -1)].set_text_props(weight='bold', color='white')


    ax.set_title('Scalability v2: Best Speedup (S) and Efficiency (E) per Process Number',
                 fontsize=14, fontweight='bold', pad=20)

    plt.tight_layout()
    plt.savefig(output, dpi=300, bbox_inches='tight')

    csv_output = output.replace('.png', '.csv')
    df_csv = pd.DataFrame(table_data, columns=processes, index=row_labels)
    df_csv.to_csv(csv_output)


if __name__ == "__main__":
    dataset_configs = [
        {
            'size': '250M',
            'csv_files': [
                {'path': 'benchmark_results_250m_v2_pack.csv', 'label': 'Pack'},
                {'path': 'benchmark_results_250m_v2_packexcl.csv', 'label': 'PackExcl'},
                {'path': 'benchmark_results_250m_v2_scatter.csv', 'label': 'Scatter'},
                {'path': 'benchmark_results_250m_v2_scatterexcl.csv', 'label': 'ScatterExcl'},
            ],
            'baseline_file': 'benchmark_results_250m_linear.csv'
        },
        {
            'size': '500M',
            'csv_files': [
                {'path': 'benchmark_results_500m_v2_pack.csv', 'label': 'Pack'},
                {'path': 'benchmark_results_500m_v2_packexcl.csv', 'label': 'PackExcl'},
                {'path': 'benchmark_results_500m_v2_scatter.csv', 'label': 'Scatter'},
                {'path': 'benchmark_results_500m_v2_scatterexcl.csv', 'label': 'ScatterExcl'},
            ],
            'baseline_file': 'benchmark_results_500m_linear.csv'
        },
        {
            'size': '1000M',
            'csv_files': [
                {'path': 'benchmark_results_1000m_v2_pack.csv', 'label': 'Pack'},
                {'path': 'benchmark_results_1000m_v2_packexcl.csv', 'label': 'PackExcl'},
                {'path': 'benchmark_results_1000m_v2_scatter.csv', 'label': 'Scatter'},
                {'path': 'benchmark_results_1000m_v2_scatterexcl.csv', 'label': 'ScatterExcl'},
            ],
            'baseline_file': 'benchmark_results_1000m_linear.csv'
        }
    ]

    folder = '../csv_results/'

    create_matrix_table(dataset_configs, folder=folder, output='scalability_table_v2.png')