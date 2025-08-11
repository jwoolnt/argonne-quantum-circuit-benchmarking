import pandas as pd
import matplotlib.pyplot as plt


def depolarizing_rate():
    df = pd.read_csv("results/depolarizing_rate/aggregate.csv")

    # Plot
    plt.figure(figsize=(10, 6))

    for rate, group in df.groupby(by="depolarizing_rate"):
        plt.plot(group["num_qubits"], group["count"] / 10, label=f"{rate * 100}%", marker='o')

    plt.xlabel("Number of Qubits")
    plt.ylabel("GHZ Fidelity (%)")
    # plt.yscale("log")
    plt.title("How does GHZ Fidelity Scale?")
    plt.legend(title="Depolarizing Rate")
    plt.grid(True)
    # plt.tight_layout()
    plt.savefig("results/plot.png")
    plt.show()


if __name__ == "__main__":
	depolarizing_rate()
