//===----------------------------------------------------------------------===//
// Copyright 2021-2024 Intel Corporation.
//
// This software and the related documents are Intel copyrighted materials, and
// your use of them is governed by the express license under which they were
// provided to you ("License"). Unless the License provides otherwise, you may
// not use, modify, copy, publish, distribute, disclose or transmit this
// software or the related documents without Intel's prior written permission.
//
// This software and the related documents are provided as is, with no express
// or implied warranties, other than those that are expressly stated in the
// License.
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

/// Quantum Runtime Library APIs
#include <quantum_full_state_simulator_backend.h>

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

const int N = 3;
const double PI = 3.141592653;

qbit Q[N];
cbit C[N];

/* Array to hold dynamic parameters for quantum algorithm */
double MyVariableParams[3];

quantum_kernel void prepAll() {
  for (int I = 0; I < N; I++) {
    PrepZ(Q[I]);
  }
}

quantum_kernel void measAll() {
  for (int I = 0; I < N; I++) {
    MeasZ(Q[I], C[I]);
  }
}

quantum_kernel void qfoo() {
  RX(Q[0], MyVariableParams[0]);
  RY(Q[1], MyVariableParams[1]);
  RZ(Q[2], MyVariableParams[2]);
}

int main() {
  /// Setup quantum device
  iqsdk::IqsConfig iqs_config(/*num_qubits*/ N,
                              /*simulation_type*/ "noiseless");
  iqsdk::FullStateSimulator iqs_device(iqs_config);
  if (iqsdk::QRT_ERROR_SUCCESS != iqs_device.ready()) {
    return 1;
  }

  srand(time(0));

  prepAll();

  for (int I = 0; I < N; I++) {
    MyVariableParams[0] = static_cast<double>(rand()) / static_cast<double>(RAND_MAX / PI);
    MyVariableParams[1] = static_cast<double>(rand()) / static_cast<double>(RAND_MAX / PI);
    MyVariableParams[2] = static_cast<double>(rand()) / static_cast<double>(RAND_MAX / PI);
    std::cout << "-----------------------------------------------------------\n";
    std::cout << "Iteration# " << I << "...\n";
    std::cout << "Angles generated: " << MyVariableParams[0] << "  " << MyVariableParams[1] << "  " << MyVariableParams[2] << "\n";
    std::cout << "-----------------------------------------------------------\n";
    qfoo();
  }

  measAll();

  for (int I = 0; I < N; I++) {
    std::cout << "Qubit " << I << " : " << (bool)C[I] << "\n";
  }

  return 0;
}
