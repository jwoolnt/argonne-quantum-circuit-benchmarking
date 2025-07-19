import intelqsdk.cbindings

from compile import compileAndLoad


SDK_NAME = "example"


compileAndLoad(SDK_NAME)

iqs_config = intelqsdk.cbindings.IqsConfig()
iqs_config.num_qubits = 20
iqs_config.simulation_type = "noiseless"
iqs_device = intelqsdk.cbindings.FullStateSimulator(iqs_config)
iqs_device.ready()

qbit_ref = intelqsdk.cbindings.RefVec()
for i in range(iqs_config.num_qubits):
    qbit_ref.append(intelqsdk.cbindings.QbitRef("qubit_register", i, SDK_NAME).get_ref())

intelqsdk.cbindings.callCppFunction("example_kernel", SDK_NAME)

# probabilities = iqs_device.getProbabilities(qbit_ref)
# intelqsdk.cbindings.FullStateSimulator.displayProbabilities(probabilities, qbit_ref)

zero_index = intelqsdk.cbindings.QssIndex(iqs_config.num_qubits * "0")
one_index = intelqsdk.cbindings.QssIndex(iqs_config.num_qubits * "1")

index_vec = intelqsdk.cbindings.QssIndexVec()
index_vec.append(zero_index)
index_vec.append(one_index)

probabilities_2 = iqs_device.getProbabilities(qbit_ref, index_vec, 0.1)
intelqsdk.cbindings.FullStateSimulator.displayProbabilities(probabilities_2)
