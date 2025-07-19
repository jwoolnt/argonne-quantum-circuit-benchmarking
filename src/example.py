import intelqsdk.cbindings as iqsdk

from qprep import compileAndLoad


SDK_NAME = "ghz"
NUM_QUBITS = 5
SIMULATION_TYPE = "noiseless"
FULL_STATE_PRINT = True
FULLSTATE_PRINT_THRESHOLD = 6
SINGLESTATE_PRINT = True
SINGLESTATE_PRINT_THRESHOLD = 0.1
SAMPLE_PRINT = True
NUM_SAMPLES = 10


compileAndLoad(SDK_NAME, replace=False)


iqs_config = iqsdk.IqsConfig()
iqs_config.num_qubits = NUM_QUBITS
iqs_config.simulation_type = SIMULATION_TYPE
iqs_device = iqsdk.FullStateSimulator(iqs_config)
iqs_device.ready()


iqsdk.callCppFunction(f"ghz_{iqs_config.num_qubits}", SDK_NAME)


qbit_ref = iqsdk.RefVec()
for i in range(iqs_config.num_qubits):
    qbit_ref.append(iqsdk.QbitRef("qubit_register", i, SDK_NAME).get_ref())

if FULL_STATE_PRINT:
    if iqs_config.num_qubits < FULLSTATE_PRINT_THRESHOLD:
        probabilities = iqs_device.getProbabilities(qbit_ref)
        iqsdk.FullStateSimulator.displayProbabilities(probabilities, qbit_ref)
    else:
        print(f"iqs_config.num_qubits > {FULLSTATE_PRINT_THRESHOLD - 1}, skipping full state print")

if SINGLESTATE_PRINT:
    zero_index = iqsdk.QssIndex(iqs_config.num_qubits * "0")
    one_index = iqsdk.QssIndex(iqs_config.num_qubits * "1")

    index_vec = iqsdk.QssIndexVec()
    index_vec.append(zero_index)
    index_vec.append(one_index)

    specific_probabilities = iqs_device.getProbabilities(qbit_ref, index_vec, SINGLESTATE_PRINT_THRESHOLD)
    iqsdk.FullStateSimulator.displayProbabilities(specific_probabilities)

if SAMPLE_PRINT:
    samples = iqs_device.getSamples(NUM_SAMPLES, qbit_ref)
    iqsdk.FullStateSimulator.displaySamples(samples)
