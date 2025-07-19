import intelqsdk.cbindings as iqsdk

from qprep import compileAndLoad


SDK_NAME = "ghz"
TOTAL_QUBITS = 20
SIMULATION_TYPE = "depolarizing"
NUM_SAMPLES = 10


compileAndLoad(SDK_NAME, replace=False)


iqs_config = iqsdk.IqsConfig()
iqs_config.simulation_type = SIMULATION_TYPE
iqs_device = iqsdk.FullStateSimulator()

cbit_ref = [
    iqsdk.CbitRef("cbit_register", i, SDK_NAME) for i in range(TOTAL_QUBITS)
]

for num_qubits in range(1, TOTAL_QUBITS + 1):
    iqs_config.num_qubits = num_qubits
    for sample_num in range(NUM_SAMPLES):
        iqs_device.initialize(iqs_config)
        iqs_device.ready()

        iqsdk.callCppFunction(f"ghzM_{num_qubits}", SDK_NAME)
        iqs_device.wait()

        state = "".join([
            "1" if cbit_ref[i].value() else "0" for i in range(num_qubits)
        ])
        state = f"|{state}>"
        print(num_qubits, sample_num, state)
