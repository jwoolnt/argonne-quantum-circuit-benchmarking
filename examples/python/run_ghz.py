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

import os
import intelqsdk.cbindings
from openqasm_bridge.v2 import translate


compiler_path = os.path.dirname(os.path.dirname(__file__)) + "/intel-quantum-compiler"

qasm_example = """
OPENQASM 2.0;
qreg qubit_register[5];
h qubit_register[0];
cx qubit_register[0],qubit_register[1];
cx qubit_register[1],qubit_register[2];
cx qubit_register[2],qubit_register[3];
cx qubit_register[3],qubit_register[4];
"""

translated = translate(qasm_example, kernel_name='my_kernel')

with open('ghz.cpp', 'w', encoding='utf8') as output_file:
    for line in translated:
        output_file.write(line + "\n")

sdk_name = "ghz"
# Compile program
intelqsdk.cbindings.compileProgram(compiler_path, "ghz.cpp", "-s", sdk_name)
iqs_config = intelqsdk.cbindings.IqsConfig()
iqs_config.num_qubits = 5
iqs_config.simulation_type = "noiseless"
iqs_device = intelqsdk.cbindings.FullStateSimulator(iqs_config)
iqs_device.ready()
intelqsdk.cbindings.callCppFunction("my_kernel", sdk_name)
qbit_ref = intelqsdk.cbindings.RefVec()
for i in range(5):
    qbit_ref.append(intelqsdk.cbindings.QbitRef("qubit_register", i, sdk_name).get_ref())
probabilities = iqs_device.getProbabilities(qbit_ref)
intelqsdk.cbindings.FullStateSimulator.displayProbabilities(probabilities, qbit_ref)

zero_index = intelqsdk.cbindings.QssIndex("00000")
one_index = intelqsdk.cbindings.QssIndex("11111")
index_vec = intelqsdk.cbindings.QssIndexVec()
index_vec.append(zero_index)
index_vec.append(one_index)
probabilities_2 = iqs_device.getProbabilities(qbit_ref, index_vec, 0.1)
intelqsdk.cbindings.FullStateSimulator.displayProbabilities(probabilities_2)
