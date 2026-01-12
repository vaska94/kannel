#!/usr/bin/env python3
"""
Simple plotting script for Kamex benchmarks.
Replaces gnuplot for systems where it's not available.

Usage: plot.py <output_base> <xlabel> <ylabel> <datafile> [title] [datafile2] [title2] ...
"""

import sys
import os

def plot(output_base, xlabel, ylabel, data_files):
    """Generate PNG and EPS plots from data files."""
    try:
        import matplotlib
        matplotlib.use('Agg')  # Non-interactive backend
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not available, skipping plot generation", file=sys.stderr)
        return False

    plt.figure(figsize=(10, 6))

    for datafile, title in data_files:
        if not os.path.exists(datafile):
            print(f"Warning: {datafile} not found, skipping", file=sys.stderr)
            continue

        x_vals = []
        y_vals = []
        with open(datafile, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        x_vals.append(float(parts[0]))
                        y_vals.append(float(parts[1]))
                    except ValueError:
                        continue

        if x_vals and y_vals:
            label = title if title else None
            plt.plot(x_vals, y_vals, label=label)

    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    if any(title for _, title in data_files):
        plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    # Save PNG
    plt.savefig(f"{output_base}.png", dpi=100)

    # Save EPS/PS
    plt.savefig(f"{output_base}.ps", format='eps')

    plt.close()
    return True


def main():
    if len(sys.argv) < 5:
        print(f"Usage: {sys.argv[0]} <output_base> <xlabel> <ylabel> <datafile> [title] [datafile2] [title2] ...")
        sys.exit(1)

    output_base = sys.argv[1]
    xlabel = sys.argv[2]
    ylabel = sys.argv[3]

    # Parse data files and titles
    data_files = []
    args = sys.argv[4:]
    i = 0
    while i < len(args):
        datafile = args[i]
        title = args[i + 1] if i + 1 < len(args) and not args[i + 1].endswith('.dat') else ""
        if datafile:  # Skip empty filenames
            data_files.append((datafile, title))
        i += 2 if title else 1

    if plot(output_base, xlabel, ylabel, data_files):
        print(f"Generated {output_base}.png and {output_base}.ps")


if __name__ == '__main__':
    main()
