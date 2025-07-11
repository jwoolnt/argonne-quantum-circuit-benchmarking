//===----------------------------------------------------------------------===//
// Copyright 2022-2024 Intel Corporation.
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
//
// QFT
// A often used subroutine which transforms between the computatioanl basis and
// the Fourier basis.
//
// QFT can be performed by applying a repeating sequence of Hadamard gate and
// two-qubit controlled phase gates, followed by a series of SWAP gates at the
// end.
//
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <math.h>
#include <quantum_full_state_simulator_backend.h>
#include <vector>

// ----------------------------------------------------------------------------------------
// ALL QUANTUM HELPER CODE
// ----------------------------------------------------------------------------------------

const int N = 6;
qbit QubitReg[N];
cbit CReg[N];

quantum_kernel void prepZAll() {
  // Initialization of the qubits
  for (int Index = 0; Index < N; Index++)
    PrepZ(QubitReg[Index]);
}

quantum_kernel void measZAll() {
  // Measurements of all the qubits
  for (int Index = 0; Index < N; Index++)
    MeasZ(QubitReg[Index], CReg[Index]);
}

quantum_kernel void qft() {
  // qft non-recursive implementation

  // Apply H and rotations
  // Starting from qubit 0
  for (int index = 0; index < N; index++) {
    H(QubitReg[index]);
    for (int index_r = 1; index_r < N - index; index_r++) {
      double angle = 2 * (1 / M_1_PI) / std::pow(2, index_r + 1);
      CPhase(QubitReg[index + index_r], QubitReg[index], angle);
    }
  }

  // Add SWAP gates
  for (int q_index = 0; q_index < std::floor(N / 2); q_index++) {
    SWAP(QubitReg[q_index], QubitReg[N - q_index - 1]);
  }
}

quantum_kernel void qft_inverse() {
  // The inverse of QFT

  // Apply the SWAP gates
  for (int q_index = 0; q_index < std::floor(N / 2); q_index++) {
    SWAP(QubitReg[q_index], QubitReg[N - q_index - 1]);
  }

  // Starting from the last qubit
  for (int index = N - 1; index >= 0; index--) {
    // Apply the controlled phase gates first using the nagetive
    // of the angles used in QFT
    for (int index_r = N - index - 1; index_r >= 1; index_r--) {
      double angle = -2 * (1 / M_1_PI) / std::pow(2, index_r + 1);
      CPhase(QubitReg[index + index_r], QubitReg[index], angle);
    }

    // Apply the H
    H(QubitReg[index]);
  }
}

int main() {
  using namespace iqsdk;
  // Set up IQS device
  IqsConfig iqs_config;
  iqs_config.num_qubits = 6;
  FullStateSimulator iqs_device(iqs_config);
  iqs_device.ready();

  // Apply the quantum circuit
  prepZAll();
  qft();

  // Get the probability
  std::vector<double> ProbabilityRegister;
  std::vector<std::reference_wrapper<qbit>> qids;
  for (int qubit = 0; qubit < N; ++qubit) {
    qids.push_back(std::ref(QubitReg[qubit]));
  }

  ProbabilityRegister = iqs_device.getProbabilities(qids);

  FullStateSimulator::displayProbabilities(ProbabilityRegister, qids);

  measZAll();

  return 0;
}
