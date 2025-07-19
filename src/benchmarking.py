from time import time

import intelqsdk.cbindings as iqsdk

from qprep import compileAndLoad
from qstate import cbits_to_state


SDK_NAME = "ghz"
TOTAL_QUBITS = 15 # up to 15-16 takes ~1 minute (on my laptop)
SIMULATION_TYPE = "depolarizing"
NUM_SAMPLES = 1000


compileAndLoad(SDK_NAME, replace=False)


iqs_config = iqsdk.IqsConfig()
iqs_config.simulation_type = SIMULATION_TYPE
iqs_device = iqsdk.FullStateSimulator()

cbit_ref = [
    iqsdk.CbitRef("cbit_register", i, SDK_NAME) for i in range(TOTAL_QUBITS)
]

start_time = time()
print("Collecting Samples...")

for num_qubits in range(1, TOTAL_QUBITS + 1):
    iqs_config.num_qubits = num_qubits
    for sample_num in range(NUM_SAMPLES):
        iqs_device.initialize(iqs_config)
        iqs_device.ready()

        iqsdk.callCppFunction(f"ghzM_{num_qubits}", SDK_NAME)
        iqs_device.wait()

        state = cbits_to_state(cbit_ref, num_qubits, True)
        # print(num_qubits, sample_num, state)
    print(f"{num_qubits}-Qubit Samples Complete")

end_time = time()
print("Sample Collection Complete", end_time - start_time)
