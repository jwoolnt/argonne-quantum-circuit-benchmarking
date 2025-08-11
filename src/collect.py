import intelqsdk.cbindings as iqsdk
import pandas as pd

from run import SDKManager
from state import cbits_to_state, bucket_state_n


def depolarizing_rate(
		max_qubits: int,
		depolarizing_rates: list[float],
		num_samples: int
):
	iqs_config = iqsdk.IqsConfig(1, "depolarizing")
	sdk_manager = SDKManager("ghz", 20, 20)

	for num_qubits in range(1, max_qubits + 1):
		print(f"{num_qubits}-Qubit Samples...")
		iqs_config.num_qubits = num_qubits

		for depolarizing_rate in depolarizing_rates:
			print(f"{depolarizing_rate * 100}%")
			iqs_config.depolarizing_rate = depolarizing_rate
			sdk_manager.configure(iqs_config)

			for sample_num in range(num_samples):
				sdk_manager.run(f"ghzM_{num_qubits}")
				state = cbits_to_state(sdk_manager.cbits, num_qubits, True)
				yield {
					"num_qubits": num_qubits,
					"depolarizing_rate": depolarizing_rate,
					"sample_num": sample_num,
					"state": state
				}


if __name__ == "__main__":
	df = pd.DataFrame([data for data in depolarizing_rate(5, [0, 0.1], 10)])
	df.to_csv("results/test.csv", index=False)
