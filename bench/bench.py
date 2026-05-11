import csv
import re

import matplotlib.pyplot as plt
import numpy as np


def extract_times(filename):
    times = []
    with open(filename, "r") as f:
        for line in f:
            m = re.search(r"time=([0-9.]+)\s*ms", line)
            if m:
                times.append(float(m.group(1)))
    return np.array(times)


def summarize(name, arr):
    return {
        "name": name,
        "count": len(arr),
        "mean": float(np.mean(arr)),
        "median": float(np.median(arr)),
        "std": float(np.std(arr)),
        "min": float(np.min(arr)),
        "p90": float(np.percentile(arr, 90)),
        "p95": float(np.percentile(arr, 95)),
        "p99": float(np.percentile(arr, 99)),
        "max": float(np.max(arr)),
    }


def trim_to_p99(arr):
    cutoff = np.percentile(arr, 99)
    return arr[arr <= cutoff]


dpdk = extract_times("data/dpdk.txt")
kernel = extract_times("data/kernel.txt")

dpdk_stats = summarize("DPDK", dpdk)
kernel_stats = summarize("Kernel", kernel)

dpdk_p99 = trim_to_p99(dpdk)
kernel_p99 = trim_to_p99(kernel)

# Save summary stats to CSV
with open("ping_summary.csv", "w", newline="") as f:
    writer = csv.DictWriter(
        f,
        fieldnames=[
            "name",
            "count",
            "mean",
            "median",
            "std",
            "min",
            "p90",
            "p95",
            "p99",
            "max",
        ],
    )
    writer.writeheader()
    writer.writerow(dpdk_stats)
    writer.writerow(kernel_stats)

# 1. Line plot (full data)
plt.figure(figsize=(10, 5))
plt.plot(dpdk, label="DPDK")
plt.plot(kernel, label="Kernel")
plt.xlabel("Ping index")
plt.ylabel("Latency (ms)")
plt.title("Ping latency over time")
plt.legend()
plt.tight_layout()
plt.savefig("ping_line_plot.png", dpi=300, bbox_inches="tight")
plt.close()

# 2. Histogram (full data)
plt.figure(figsize=(10, 5))
plt.hist(dpdk, bins=30, alpha=0.6, label="DPDK")
plt.hist(kernel, bins=30, alpha=0.6, label="Kernel")
plt.xlabel("Latency (ms)")
plt.ylabel("Count")
plt.title("Latency histogram (full data)")
plt.legend()
plt.tight_layout()
plt.savefig("ping_histogram_full.png", dpi=300, bbox_inches="tight")
plt.close()

# 3. Boxplot (full data)
plt.figure(figsize=(8, 5))
plt.boxplot([dpdk, kernel], tick_labels=["DPDK", "Kernel"])
plt.ylabel("Latency (ms)")
plt.title("Latency boxplot (full data)")
plt.tight_layout()
plt.savefig("ping_boxplot_full.png", dpi=300, bbox_inches="tight")
plt.close()

# 4. Histogram (trimmed to p99)
plt.figure(figsize=(10, 5))
plt.hist(dpdk_p99, bins=30, alpha=0.6, label="DPDK")
plt.hist(kernel_p99, bins=30, alpha=0.6, label="Kernel")
plt.xlabel("Latency (ms)")
plt.ylabel("Count")
plt.title("Latency histogram (up to p99)")
plt.legend()
plt.tight_layout()
plt.savefig("ping_histogram_p99.png", dpi=300, bbox_inches="tight")
plt.close()

# 5. Boxplot (trimmed to p99)
plt.figure(figsize=(8, 5))
plt.boxplot([dpdk_p99, kernel_p99], tick_labels=["DPDK", "Kernel"])
plt.ylabel("Latency (ms)")
plt.title("Latency boxplot (up to p99)")
plt.tight_layout()
plt.savefig("ping_boxplot_p99.png", dpi=300, bbox_inches="tight")
plt.close()

print("Saved files:")
print("- ping_line_plot.png")
print("- ping_histogram_full.png")
print("- ping_boxplot_full.png")
print("- ping_histogram_p99.png")
print("- ping_boxplot_p99.png")
print("- ping_summary.csv")
