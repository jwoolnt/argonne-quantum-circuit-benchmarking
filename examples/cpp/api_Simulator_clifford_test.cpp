//===----------------------------------------------------------------------===//
// Copyright 2024 Intel Corporation.
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
#include <quantum_clifford_simulator_backend.h>

const int total_qubits = 2;
qbit qubit_register[total_qubits];

quantum_kernel void ghz_total_qubits() {
  for (int i = 0; i < total_qubits; i++) {
    PrepZ(qubit_register[i]);
  }

  H(qubit_register[0]);

  for (int i = 0; i < total_qubits - 1; i++) {
    CNOT(qubit_register[i], qubit_register[i + 1]);
  }
}

int main() {
  int seed = 165498721;

  iqsdk::CliffordSimulatorConfig clifford_config(seed);
  iqsdk::CliffordSimulator quantum_8086;
  std::cout << std::endl << "Test:  default constructor" << std::endl;
  if (iqsdk::QRT_ERROR_SUCCESS != quantum_8086.ready()) {
    std::cout << "quantum_8086 is not ready; expected." << std::endl;
  }

  std::cout << std::endl << "testing . . . isValid" << std::endl;
  if (quantum_8086.isValid()) {
    std::cout << "quantum_8086 reports it is valid; NOT expected!!"
              << std::endl;
  } else {
    std::cout << "quantum_8086 reports it is not valid; expected." << std::endl;
  }

  std::cout << std::endl << "testing . . . initialize" << std::endl;
  quantum_8086.initialize(clifford_config);
  if (quantum_8086.isValid()) {
    std::cout << "quantum_8086 reports it is valid; expected." << std::endl;
  } else {
    std::cout << "quantum_8086 reports it is not valid; NOT expected!!"
              << std::endl;
  }

  std::cout << std::endl << "testing . . .  ready" << std::endl;
  if (iqsdk::QRT_ERROR_SUCCESS != quantum_8086.ready()) {
    std::cout << "quantum_8086 is not ready; NOT expected!!" << std::endl;
  }

  std::cout << std::endl << "testing . . .  printVerbose" << std::endl;
  quantum_8086.printVerbose(true);

  // get references to qbits
  std::vector<std::reference_wrapper<qbit>> qids;
  for (int id = 0; id < total_qubits; ++id) {
    qids.push_back(std::ref(qubit_register[id]));
  }

  ghz_total_qubits();

  // specifying all the possible two qubit Pauli strings
  std::string two_qubit_paulis[16] = {"II", "IX", "IY", "IZ", "XI", "XX",
                                      "XY", "XZ", "YI", "YX", "YY", "YZ",
                                      "ZI", "ZX", "ZY", "ZZ"};

  // getting and printing the expectation value for each of the Pauli strings
  for (size_t i = 0; i < 16; i++) {
    std::string pauli_string = two_qubit_paulis[i];
    double result = quantum_8086.getExpectationValue(qids, pauli_string);
    std::cout << "Expectation value for operator " << pauli_string << " is "
              << result << "\n";
  }

  return 0;
}
