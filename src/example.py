import intelqsdk.cbindings


compiler_path = "/opt/intel/quantum-sdk/latest/intel-quantum-compiler"
sdk_name = "example"

intelqsdk.cbindings.compileProgram(compiler_path, "example.cpp", "-o qbuild -P json -s", sdk_name)
intelqsdk.cbindings.loadSdk(f"qbuild/{sdk_name}.so", sdk_name)

iqs_config = intelqsdk.cbindings.IqsConfig()
iqs_config.num_qubits = 5
iqs_config.simulation_type = "noiseless"
iqs_device = intelqsdk.cbindings.FullStateSimulator(iqs_config)
iqs_device.ready()

qbit_ref = intelqsdk.cbindings.RefVec()
for i in range(5):
    qbit_ref.append(intelqsdk.cbindings.QbitRef("qubit_register", i, sdk_name).get_ref())

intelqsdk.cbindings.callCppFunction("example_kernel", sdk_name)

probabilities = iqs_device.getProbabilities(qbit_ref)
intelqsdk.cbindings.FullStateSimulator.displayProbabilities(probabilities, qbit_ref)

zero_index = intelqsdk.cbindings.QssIndex("00000")
one_index = intelqsdk.cbindings.QssIndex("11111")

index_vec = intelqsdk.cbindings.QssIndexVec()
index_vec.append(zero_index)
index_vec.append(one_index)

probabilities_2 = iqs_device.getProbabilities(qbit_ref, index_vec, 0.1)
intelqsdk.cbindings.FullStateSimulator.displayProbabilities(probabilities_2)
