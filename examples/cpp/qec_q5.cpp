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

#include <iostream>
#include <quantum_full_state_simulator_backend.h>

// Allocate 5 global qubits.
qbit qumem[5];
cbit cmem[5];

//
// Quantum Basic Block (QBB) to set the initial state of qubit0.
//
quantum_kernel void initialize_qubit_state0() { PrepZ(qumem[0]); }
quantum_kernel void initialize_qubit_state1() {
  PrepZ(qumem[0]);
  X(qumem[0]);
}

//
// Quantum Basic Block (QBB) to initialize the repetition code - 3 state.
//
quantum_kernel void build_repetition3_code() {
  for (int idx = 1; idx < 5; idx++)
    PrepZ(qumem[idx]);

  CNOT(qumem[0], qumem[1]);
  CNOT(qumem[0], qumem[2]);
}

//
// Quantum Basic Block (QBB) to perform the error syndrome extraction
//
quantum_kernel void decode() {
  CNOT(qumem[0], qumem[3]);
  CNOT(qumem[1], qumem[3]);
  CNOT(qumem[1], qumem[4]);
  CNOT(qumem[2], qumem[4]);

  MeasZ(qumem[3], cmem[3]);
  MeasZ(qumem[4], cmem[4]);
}

//
// Quantumn Basic Block (QBB) to perform the bit-flip correction on qubit0.
//
quantum_kernel void flip_qubit0() { X(qumem[0]); }

//
// Quantumn Basic Block (QBB) to perform the bit-flip correction on qubit1.
//
quantum_kernel void flip_qubit1() { X(qumem[1]); }

//
// Quantumn Basic Block (QBB) to perform the bit-flip correction on qubit2.
//
quantum_kernel void flip_qubit2() { X(qumem[2]); }

//
// Quantum Basic Block (QBB) to reset the ancillas without a bit flip action.
//
quantum_kernel void reset_ancillas() {
  PrepZ(qumem[3]);
  PrepZ(qumem[4]);
}

//
// Perform a read-out of all of the qubits and get the results in the
// ClassicalBitRegister.
//
quantum_kernel void measure_qubits() {
  for (int idx = 0; idx < 5; idx++)
    MeasZ(qumem[idx], cmem[idx]);
}

//
// Main parity check circuit loop running on the Host CPU.
//
int main() {
  using namespace iqsdk;
  IqsConfig iqs_config;
  iqs_config.num_qubits = 5;
  FullStateSimulator iqs_device(iqs_config);
  iqs_device.ready();
  auto data_bit_orig = 1;
  //
  // Prepare the qubit in the initial state.
  //
  if (data_bit_orig == 1)
    initialize_qubit_state1();
  else
    initialize_qubit_state0();

  // Build the 3 qubit repetition code.
  // qumem[0]     - the bit we wish to preserve by the code.
  // qumem[1-2]   - the bits in the repetition block.
  // qumem[3-4]   - the ancilla qubits for parity measurement.
  build_repetition3_code();

  for (int cycles = 0; cycles < 5; cycles++) {

    std::cout
        << "-----------------------------------------------------------\n";
    std::cout << "Cycle#" << cycles << "\n";
    std::cout
        << "-----------------------------------------------------------\n";

    // Apply a decode block to prepare to measure any bit-flip errors.
    decode();

    if (cmem[3] && !cmem[4]) // |10> - first qubit was flipped.
      flip_qubit0();
    else if (cmem[3] && cmem[4]) // |11> - second qubit was flipped.
      flip_qubit1();
    else if (!cmem[3] && cmem[4]) // |01> - third qubit was flipped.
      flip_qubit2();

    reset_ancillas(); // Resetting the ancillas
  }

  //
  // Perform a measurement of the data qubit after running detection
  // circuit for 5 rounds.
  //
  measure_qubits();

  // Did we do a good job of protecting the qubit?
  if (cmem[0] == data_bit_orig)
    return 0;

  return 1;
}
