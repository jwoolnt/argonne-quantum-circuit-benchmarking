import intelqsdk.cbindings as iqsdk
import pandas as pd

from run import SDKManager
from state import cbits_to_state, bucket_state_n

from globals import RESULTS_FOLDER


def collect(
		sdk_name,
		func,
		*args,
		results_folder: str = RESULTS_FOLDER,
		file_name: str = None,
		**kwargs
):
	file_path = f"{results_folder}/{sdk_name}/{file_name or func.__name__}.csv"
	df = pd.DataFrame([data for data in func(sdk_name, *args, **kwargs)])
	df.to_csv(file_path, index=False)


def depolarizing_rate(
		sdk_name: str,
		max_qubits: int = 15,
		depolarizing_rates: list[float] = [0.0, 0.0001, 0.001, 0.01, 0.1],
		num_samples: int = 1000
):
	iqs_config = iqsdk.IqsConfig(1, "depolarizing")
	sdk_manager = SDKManager(sdk_name, 20, 20)

	for num_qubits in range(1, max_qubits + 1):
		print(f"{num_qubits}-Qubit Samples...")
		iqs_config.num_qubits = num_qubits
		bucket_states = bucket_state_n(num_qubits)

		for depolarizing_rate in depolarizing_rates:
			print(f"{depolarizing_rate * 100}%")
			iqs_config.depolarizing_rate = depolarizing_rate
			sdk_manager.configure(iqs_config)

			count = 0
			for _ in range(num_samples):
				sdk_manager.run(f"ghzM_{num_qubits}")
				state = cbits_to_state(sdk_manager.cbits, num_qubits)
				count += state in bucket_states

			yield {
				"num_qubits": num_qubits,
				"depolarizing_rate": depolarizing_rate,
				"count": count
			}


def correlation(sdk_name: str, num_samples: int = 100):
	from subprocess import run

	percentage = 0
	for sample_num in range(num_samples):
		if sample_num % (num_samples / 10) == 0:
			print(f"{percentage}%...")
			percentage += 10

		result = run(
			[f"qbuild/{sdk_name}"],
			capture_output=True,
			text=True
		)
		s = result.stdout

		yield {
			f"qubit_{i}":s[i] for i in range(len(s)) if s[i] in {"0", "1"}
		}
	print("100%")


if __name__ == "__main__":
	collect("ghz_error", correlation)
