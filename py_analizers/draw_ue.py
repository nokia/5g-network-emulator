#!/usr/bin/env python3
import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from glob import glob


def find_ue_logs(base_path):
    ue_dirs = glob(os.path.join(base_path, '*', 'ue'))
    if not ue_dirs:
        print(f"[ERROR] No UE log directories found in {base_path}")
        sys.exit(1)
    latest_dir = max(ue_dirs, key=os.path.getmtime)
    log_files = sorted(glob(os.path.join(latest_dir, 'ue_log_*.txt')))
    return latest_dir, log_files


def parse_log_file_with_tx(file_path):
    data = {
        'x': [], 'y': [],
        'rsrp_ul': [], 'rsrp_dl': [],
        'sinr_ul': [], 'sinr_dl': [],
        'rul': [], 'rdl': [], 'ri': []
    }

    with open(file_path, 'r') as f:
        tx_flag = None
        for line in f:
            tokens = line.split()
            temp = {}
            for token in tokens:
                if ':' in token:
                    k, v = token.split(':', 1)
                    try:
                        temp[k] = float(v)
                    except ValueError:
                        pass
            if 'tx' in temp:
                tx_flag = int(temp['tx'])
            if 'sinr' in temp and tx_flag is not None:
                if tx_flag == 1:
                    data['sinr_ul'].append(temp['sinr'])
                else:
                    data['sinr_dl'].append(temp['sinr'])
            if 'rsrp' in temp and tx_flag is not None:
                if tx_flag == 1:
                    data['rsrp_ul'].append(temp['rsrp'])
                else:
                    data['rsrp_dl'].append(temp['rsrp'])
            for key in ['x', 'y', 'rul', 'rdl', 'ri']:
                if key in temp:
                    data[key].append(temp[key])
    return data


def plot_combined_trajectory(all_data, outdir):
    plt.figure(figsize=(10, 10))
    cmap = plt.get_cmap("tab10")
    for i, data in enumerate(all_data):
        x, y = data['x'], data['y']
        n = min(len(x), len(y)) # Trim to the shortest length
        x, y = x[:n], y[:n]
        if not x or not y:
            continue
        plt.plot(x, y, label=f"UE {i}", alpha=0.6, color=cmap(i % cmap.N))
        plt.plot(x[0], y[0], 'o', color=cmap(i % cmap.N))
        plt.plot(x[-1], y[-1], 's', color=cmap(i % cmap.N))
    plt.plot(0, 0, 'kp', label='Base Station', markersize=10)
    plt.axis('equal')
    plt.grid(True)
    plt.legend()
    plt.title("UE Trajectories")
    plt.tight_layout()
    path = os.path.join(outdir, "trajectory.png")
    plt.savefig(path)
    print(f"[SAVED] {path}")


def plot_histograms_ul_dl(all_data, key_ul, key_dl, title, xlabel, outname, bins, outdir):
    fig, axs = plt.subplots(2, 1, figsize=(10, 10), sharex=True)
    cmap = plt.get_cmap("tab10")
    for i, data in enumerate(all_data):
        ul_vals = np.asarray(data[key_ul], dtype=float)
        dl_vals = np.asarray(data[key_dl], dtype=float)

        ul_vals = ul_vals[np.isfinite(ul_vals)]
        dl_vals = dl_vals[np.isfinite(dl_vals)]

        if ul_vals.size:
            axs[0].hist(ul_vals, bins=bins, alpha=0.5, label=f"UE {i}", color=cmap(i % cmap.N), density=True, edgecolor='black')
        if dl_vals.size:
            axs[1].hist(dl_vals, bins=bins, alpha=0.5, label=f"UE {i}", color=cmap(i % cmap.N), density=True, edgecolor='black')
    axs[0].set_title(f"{title} (UL)")
    axs[1].set_title(f"{title} (DL)")
    axs[1].set_xlabel(xlabel)
    axs[0].legend()
    axs[1].legend()
    axs[0].grid(True)
    axs[1].grid(True)
    fig.tight_layout()
    path = os.path.join(outdir, outname)
    plt.savefig(path)
    print(f"[SAVED] {path}")


def plot_histograms_per_ue(all_data, key, title, xlabel, outname, outdir, bins):
    plt.figure(figsize=(10, 6))
    cmap = plt.get_cmap("tab10")
    for i, data in enumerate(all_data):
        vals = np.asarray(data[key], dtype=float)
        vals = vals[np.isfinite(vals)]
        if not vals.size:
            continue
        plt.hist(vals, bins=bins, alpha=0.5, label=f"UE {i}", density=True, edgecolor='black', color=cmap(i % cmap.N))
    plt.title(title)
    plt.xlabel(xlabel)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    path = os.path.join(outdir, outname)
    plt.savefig(path)
    print(f"[SAVED] {path}")


def main():
    script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
    logs_base = os.path.join(script_dir, '..', 'logs')
    ue_dir, log_files = find_ue_logs(logs_base)
    log_folder = os.path.basename(os.path.dirname(ue_dir))
    out_dir = os.path.join(script_dir, "..", "results", log_folder, "ue")
    os.makedirs(out_dir, exist_ok=True)

    all_data = [parse_log_file_with_tx(f) for f in log_files]
    plot_combined_trajectory(all_data, out_dir)

    plot_histograms_ul_dl(all_data, 'sinr_ul', 'sinr_dl', 'SINR Histogram', 'SINR (dB)', 'sinr_hist_ul_dl.png', bins=40, outdir=out_dir)
    plot_histograms_ul_dl(all_data, 'rsrp_ul', 'rsrp_dl', 'RSRP Histogram', 'RSRP (dBm)', 'rsrp_hist_ul_dl.png', bins=40, outdir=out_dir)
    plot_histograms_per_ue(all_data, 'rul', 'UL Throughput Histogram', 'Throughput (Mbps)', 'rul_hist.png', outdir=out_dir, bins=30)
    plot_histograms_per_ue(all_data, 'rdl', 'DL Throughput Histogram', 'Throughput (Mbps)', 'rdl_hist.png', outdir=out_dir, bins=30)
    plot_histograms_per_ue(all_data, 'ri', 'Rank Indicator Histogram', 'Rank', 'ri_hist.png', outdir=out_dir, bins=np.arange(0.5, 5.5, 1))


if __name__ == "__main__":
    main()
