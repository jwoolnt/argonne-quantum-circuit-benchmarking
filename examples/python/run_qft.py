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

# .so file generated using QRT_Model/app_examples/qft.cpp

from intelqsdk.cbindings import *
sdk_config = "qft"
loadSdk("./qft.so", sdk_config)

# Set up IQS device
N = 6
iqs_config = IqsConfig()
iqs_config.num_qubits = N
iqs_config.simulation_type = "noiseless"
iqs_config.synchronous = False
iqs_device = FullStateSimulator(iqs_config)
iqs_device.ready()

# Create python equivalent of the cpp objects used by the library
qids = RefVec()
cbits = []
for i in range(N):
    cbits.append(CbitRef("CReg", i, sdk_config))
    qids.append(QbitRef("QubitReg", i, sdk_config).get_ref())

# Run the qft circuit
# Prepare all qubits in the 0 state
callCppFunction("prepZAll", sdk_config)
# Apply QFT
callCppFunction("qft", sdk_config)
# Apply the inverse of QFT, effectively applying an Identity
callCppFunction("qft_inverse", sdk_config)

# Get the probablities
probs = iqs_device.getProbabilities(qids)
amplitudes = iqs_device.getAmplitudes(qids)
callCppFunction("measZAll", sdk_config)

# Wait instruction is needed before printing the measurement results
# since the device is set up to be asynchoronous
iqs_device.wait()
print("\nMeasurements:")
for cbit in cbits:
    print(cbit.value())

print("\nProbabilities printed with QRT API")
# Expect to see |0000> to have a probablity of 1
# since we have applied an identity
FullStateSimulator.displayProbabilities(probs, qids)

print("Amplitudes printed with QRT API")
FullStateSimulator.displayAmplitudes(amplitudes, qids)

iqs_device.wait()
