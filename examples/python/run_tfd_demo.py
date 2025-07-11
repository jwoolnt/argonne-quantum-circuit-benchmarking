#===----------------------------------------------------------------------===//
# Copyright 2021-2024 Intel Corporation.
##
# This software and the related documents are Intel copyrighted materials, and
# your use of them is governed by the express license under which they were
# provided to you ("License"). Unless the License provides otherwise, you may
# not use, modify, copy, publish, distribute, disclose or transmit this
# software or the related documents without Intel's prior written permission.
#
# This software and the related documents are provided as is, with no express
# or implied warranties, other than those that are expressly stated in the
# License.
#===----------------------------------------------------------------------===//

# Use tfd_q4_hybrid_demo.cpp from QRT_Model/app_examples

from intelqsdk.cbindings import *
sdk_config = "tfd"
loadSdk("./tfd_q4_hybrid_demo.so", sdk_config)
iqs_config = IqsConfig()
iqs_config.num_qubits = 4
iqs_config.simulation_type = "noiseless"
iqs_config.synchronous = False
iqs_device = FullStateSimulator(iqs_config)
iqs_device.ready()
qids = RefVec()
cbits = []
for i in range(4):
    cbits.append(CbitRef("CReg", i, sdk_config))
    qids.append(QbitRef("QubitReg", i, sdk_config).get_ref())
# Setup quantum variables
for index in range(4):
    setDoubleArray("QuantumVariableParams", index, 0.11111111111 * (index + 1), sdk_config)
callCppFunction("prepZAll", sdk_config)
callCppFunction("tfdQ4Setup", sdk_config)
probs = iqs_device.getProbabilities(qids)
probs_index = iqs_device.getProbabilities(qids, QssIndexVec(), 0.1)
single_probs = iqs_device.getSingleQubitProbs(qids)
samples = iqs_device.getSamples(3, qids)
amplitudes = iqs_device.getAmplitudes(qids)
callCppFunction("measZAll", sdk_config)
new_probs = iqs_device.getProbabilities(qids)
new_single_probs = iqs_device.getSingleQubitProbs(qids)
print("New single probs")
for prob in new_single_probs:
    print(prob)
print("New probs")
FullStateSimulator.displayProbabilities(new_probs, qids)

print("Probabilities:")
for prob in probs:
    print(prob)
print("Measurements:")
for cbit in cbits:
    print(cbit.value())

print("Probabilities printed with QRT API")
FullStateSimulator.displayProbabilities(probs, qids)

print("Samples:")
for sample in samples:
    for value in sample:
        print(value, end=" ")
    print()

print("Single Qubit Probabilities:")
for prob in single_probs:
    print(prob)

print("Amplitudes:")
for amplitude in amplitudes:
    print(amplitude)

print("Amplitudes printed with QRT API")
FullStateSimulator.displayAmplitudes(amplitudes, qids)

print("Probabilities greater than 0.1")
for pair in probs_index:
    print(pair.key().getIndex(), pair.data())

# No-op since device is synchronous
iqs_device.wait()

print("Printing Qubit Placement")
for i in range(4):
    print(f"QubitReg[{i}] -> " + str(QbitRef("QubitReg", i, sdk_config).value()))
