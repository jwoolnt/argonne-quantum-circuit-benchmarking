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


def correlation_fidelity(
        input_path: str,
        output_path: str,
        num_samples: int = 100,
        sample_size: int = 100
):
    df = pd.read_csv(f"{input_path}.csv")

    correlation = []
    fidelity = []
    for _ in range(num_samples):
         sample = df.sample(sample_size)
         correlated = sample["qubit_0"] == sample["qubit_1"]
         correct = sample.nunique(axis=1) == 1

         correlation.append(correlated.sum() / sample_size)
         fidelity.append(correct.sum() / sample_size)

    plt.figure(figsize=(10, 6))

    plt.plot(correlation, fidelity, "o")

    plt.title("How does GHZ Fidelity relate to Correlation Rate?")
    plt.xlabel("Correlation Rate (%)")
    plt.ylabel("GHZ Fidelity (%)")
    plt.grid(True)

    plt.savefig(f"{output_path}.png")


if __name__ == "__main__":
    sdk_name = "ghz_manual"
    visualize(sdk_name, correlation)
    visualize(sdk_name, correlation_fidelity)
