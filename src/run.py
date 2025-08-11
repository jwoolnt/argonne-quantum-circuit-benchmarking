import intelqsdk.cbindings as iqsdk

from prep import load


class SDKManager:
	LOADED_SDKS = {}

	def __new__(cls, sdk_name: str, *_args, **_kwargs):
		if sdk_name not in SDKManager.LOADED_SDKS:
			load(sdk_name)
			SDKManager.LOADED_SDKS[sdk_name] = super().__new__(cls)
		return SDKManager.LOADED_SDKS[sdk_name]

	def __init__(
			self,
			sdk_name: str,
			/,
			qubit_count: int = 0,
			cbit_count: int = 0,
			qubit_register_name: str = "qubit_register",
			cbit_register_name: str = "cbit_register"
		):
		self.sdk_name = sdk_name

		self.qubits = iqsdk.RefVec()
		for i in range(qubit_count):
			self.qubits.append(
				iqsdk.QbitRef(qubit_register_name, i, sdk_name).get_ref()
			)
		self.cbits = []
		for i in range(cbit_count):
			self.cbits.append(
				iqsdk.CbitRef(cbit_register_name, i, sdk_name)
			)

		self.iqs_device = None

	def configure(self, iqs_config):
		iqs_device = iqsdk.FullStateSimulator(iqs_config)
		self.iqs_device = iqs_device if iqs_device.isValid() else None

	@property
	def valid(self):
		return self.iqs_device is not None and self.iqs_device.isValid()

	def run(self, function_name: str):
		if self.valid and self.iqs_device.ready() == iqsdk.QRT_ERROR_T.QRT_ERROR_SUCCESS:
			iqsdk.callCppFunction(function_name, self.sdk_name)
			self.iqs_device.wait()


if __name__ == "__main__":
	N = 10
	dm = SDKManager("ghz", N, N)
	dm.configure(iqsdk.IqsConfig())
	for i in range(10):
		dm.run(f"ghzM_{N}")
		state = ["1" if dm.cbits[i].value() else "0" for i in range(N)]
		print("|" + "".join(state) + ">")
