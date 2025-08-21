import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

from globals import RESULTS_FOLDER


def visualize(
          sdk_name: str,
          func,
          *args,
          results_folder: str = RESULTS_FOLDER,
          input_name: str = None,
          output_name: str = None,
          **kwargs
):
    path = lambda name: f"{results_folder}/{sdk_name}/{name or func.__name__}"
    input_path = path(input_name)
    output_path = path(output_name) if output_name else input_path
    func(input_path, output_path, *args, **kwargs)


def depolarizing_rate(input_path: str, output_path: str):
    df = pd.read_csv(f"{input_path}.csv")

    plt.figure(figsize=(10, 6))

    for rate, group in df.groupby(by="depolarizing_rate"):
        plt.plot(group["num_qubits"], group["count"] / 10, label=f"{rate * 100}%", marker='o')

    plt.title("How does GHZ Fidelity Scale?")
    plt.xlabel("Number of Qubits")
    plt.ylabel("GHZ Fidelity (%)")
    plt.legend(title="Depolarizing Rate")
    plt.grid(True)

    plt.savefig(f"{output_path}.png")


def correlation(input_path: str, output_path: str):
    df = pd.read_csv(f"{input_path}.csv")

    plt.figure(figsize=(10, 6))

    sns.heatmap(df.corr(), annot=True, cmap="viridis", fmt=".2f")

    plt.savefig(f"{output_path}.png")


if __name__ == "__main__":
	visualize("ghz_error", correlation)
