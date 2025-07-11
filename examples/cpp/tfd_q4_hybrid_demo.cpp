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
//
// TFD
// A single-step version of an algorithm to generate 4-qubit Thermofield Double
// States (TFD) which are purifications of Gibbs states.
//
// References:
//
// Theory:
//
// 1. Wu, J. & Hsieh, T. H. Variational thermal quantum simulation via
// thermofield double states. Phys. Rev. Lett. 123, 220502 (2019).
//
// 2. Ho, W. W. & Hsieh, T. H. Efficient variational simulation of non-trivial
// quantum states. SciPost Phys. 6, 29 (2019).
//
// 3. Premaratne, S. P. & Matsuura, A. Y. Engineering a cost function for
// real-world implementation of a variational quantum algorithm. In Proc. 2020
// IEEE Int. Conf. Quantum Comp. Eng., 278{285 (2020).
//
// Experiment:
//
// 1. Sagastizabal, R. et al. Variational preparation of  finite-temperature
// states on a quantum computer (2021). Preprint at
// https://arxiv.org/abs/2012.03895.
//
// 2. Zhu, D. et al. Generation of thermofield double states and critical ground
// states with a quantum computer. P. Natl Acad. Sci. USA 117, 25402{25406
// (2020).
//
// 3. Francis, A. et al. Many body thermodynamics on quantum computers via
// partition function zeros (2020). Preprint at
// https://arxiv.org/abs/2009.04648.
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

/// Development mode
// #include "../../clang/include/clang/Quantum/quintrinsics.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <math.h>
#include <quantum_full_state_simulator_backend.h>
#include <vector>
// ----------------------------------------------------------------------------------------
// ALL QUANTUM HELPER CODE
// ----------------------------------------------------------------------------------------

const int N = 4;
qbit QubitReg[N];
cbit CReg[N];

/* Array to hold dynamic parameters for quantum algorithm */
double QuantumVariableParams[N];

quantum_kernel void tfdQ4Setup() {
  // Index to loop over later
  int Index = 0;

  // preparation of Bell pairs (T -> Infinity)
  for (Index = 0; Index < 2; Index++)
    RY(QubitReg[Index], 1.57079632679);
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[Index], QubitReg[Index + 2]);

  // Single qubit variational terms
  for (Index = 0; Index < N; Index++)
    RX(QubitReg[Index], QuantumVariableParams[2]);

  // Two-qubit intra-system variational terms
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[2 * Index + 1], QubitReg[2 * Index]);
  for (Index = 0; Index < 2; Index++)
    RZ(QubitReg[2 * Index], QuantumVariableParams[3]);
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[2 * Index + 1], QubitReg[2 * Index]);

  // two-qubit inter-system XX variational terms
  for (Index = 0; Index < N; Index++)
    RY(QubitReg[Index], -1.57079632679);
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[Index + 2], QubitReg[Index]);
  for (Index = 0; Index < 2; Index++)
    RZ(QubitReg[Index], QuantumVariableParams[0]);
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[Index + 2], QubitReg[Index]);
  for (Index = 0; Index < N; Index++)
    RY(QubitReg[Index], 1.57079632679);

  // two-qubit inter-system ZZ variational terms
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[Index], QubitReg[Index + 2]);
  for (Index = 0; Index < 2; Index++)
    RZ(QubitReg[Index + 2], QuantumVariableParams[1]);
  for (Index = 0; Index < 2; Index++)
    CNOT(QubitReg[Index], QubitReg[Index + 2]);
}

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

int main() {
  using namespace iqsdk;
  IqsConfig iqs_config;
  iqs_config.num_qubits = 4;
  FullStateSimulator iqs_device(iqs_config);
  iqs_device.ready();

  /// Using Fig. S4 from the Intel/Delft TFD Paper. This uses CNOT gates and X,
  /// Y, Z gates compact representation only valid for 4 qubits
  QuantumVariableParams[0] = 0.11111111111; // alpha1
  QuantumVariableParams[1] = 0.22222222222; // alpha2
  QuantumVariableParams[2] = 0.33333333333; // gamma1
  QuantumVariableParams[3] = 0.44444444444; // gamma2

  int RunCount = 0;
  bool Optimized = false;
  int MaxRuns = 4;

  while (!Optimized) {
    std::cout << "+++++++++++++ Run count# " << RunCount++
              << " +++++++++++++\n";
    prepZAll();
    tfdQ4Setup();

    std::vector<double> ProbabilityRegister;
    std::vector<std::reference_wrapper<qbit>> qids;
    for (int qubit = 0; qubit < N; ++qubit) {
      qids.push_back(std::ref(QubitReg[qubit]));
    }
    ProbabilityRegister = iqs_device.getProbabilities(qids);

    FullStateSimulator::displayProbabilities(ProbabilityRegister, qids);

    measZAll();

    /// Some arbitrary condition
    /// Update the condition based on averaged results
    if ((CReg[0]) && (CReg[1])) {
      Optimized = true;
    } else if (RunCount == MaxRuns) {
      break;
    } else {
      /// Replace with optimization function
      QuantumVariableParams[0] *= 2;
      QuantumVariableParams[1] *= 2;
      QuantumVariableParams[2] *= 2;
      QuantumVariableParams[3] *= 2;
    }
  }

  return 0;
}
