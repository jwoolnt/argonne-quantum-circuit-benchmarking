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
// Deutsch-Jozsa algorithm
//
// The Deutsch-Jozsa algorithm is a generalization of Deutsch's algorithm.
//
// Assume we have a function f : 2^N -> 2 on N bits which is either constant
// (all inputs produce the same value) or balanced (half the inputs produce
// one value, half the other). In a classical setting, the only way to
// determine whether f is constant or balanced is to check f on N/2+1 inputs
// in the worst case. In a quantum setting, we can determine the result by
// applying f only once.
//
// The algorithm applies the following circuit, where U_f is a quantum oracle
// such that U_f(|b0 .. bn 0>) = |b0 .. bn z> if and only if f(b0,..,bn) = z.
//
//
//      |qs_0>   : - PrepZ --- H -------- |---------------| --- H --- MeasZ --
//                                        |               | ---
//      |qs_1>   : - PrepZ --- H -------- |               | --- H --- MeasZ --
//                                        |      U_f      | ---
//      |qs_2>   : - PrepZ --- H -------- |               | --- H --- MeasZ --
//                                        |               | ---
//      |qout>   : - PrepZ --- X --- H -- |---------------| ------------------
//
// In that case, the output results will always produce all 0s if and only if
// f is a constant function.
//
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

/// Quantum Runtime Library APIs
#include <quantum_full_state_simulator_backend.h>

#include <iostream>
#include <vector>

const int N = 6;
qbit Qs[N];
qbit Qout;

// Prep each of the N input qubits (Qs) in |+> state
// Prep the output circuit (Qout) in |-> state
quantum_kernel void prepInputs() {
  for (int i = 0; i < N; i++) {
    PrepZ(Qs[i]);
    H(Qs[i]);
  }
  PrepZ(Qout);
  X(Qout);
  H(Qout);
}

/* After applying the unitary oracle, process
 * the results by applying H and measuring each
 * input qubit (Qs).
 */
quantum_kernel std::vector<bool> process() {
  cbit cs[N];
  for (int i = 0; i < N; i++) {
    H(Qs[i]);
    MeasZ(Qs[i], cs[i]);
  }
  release_quantum_state();

  std::vector<bool> results;
  for (int i = 0; i < N; i++) {
    results.push_back(cs[i]);
  }
  return results;
}

/* Classical oracles
 */
quantum_kernel void constant0() { return; }
quantum_kernel void constant1() { X(Qout); }
// A balanced function obtained by simply having Qout reflect the value of the
// first bit in the input string
//
// 0,b1,..,bn |-> 0
// 1,b1,..,bn |-> 1
quantum_kernel void balancedByQ0() { CNOT(Qs[0], Qout); }
// A balanced function outputting the parity of the input string.
//
// 000 , 011 , 101 , 110 |-> 0
// 001 , 100 , 010 , 111 |-> 1
quantum_kernel void balancedParity() {
  for (int i = 0; i < N; i++) {
    CNOT(Qs[i], Qout);
  }
}

void printSample(std::vector<bool> sample) {
  std::cout << "|";
  for (bool x : sample) {
    std::cout << x;
  }
  std::cout << ">";
}

/* The input is a function pointer to a quantum kernel that applies
 * either a constant or balanced classical oracle.
 */
void deutschJozsa(void (*qk_oracle)()) {
  prepInputs();
  qk_oracle();
  std::vector<bool> sample = process();

  // If the function is constant, WILL get all 0 state |0..0>
  // If the function is balanced, WILL NOT get the all 0 state |0..0>
  std::cout << "Got sample: ";
  printSample(sample);
  bool all0 = true;
  for (int i = 0; i < N; i++) {
    all0 = all0 && (sample[i] == false);
  }
  if (all0)
    std::cout << ": function is constant\n";
  else
    std::cout << ": function is balanced\n";
}

int main() {
  /// Setup quantum device
  iqsdk::IqsConfig iqs_config(/*num_qubits*/ N,
                              /*simulation_type*/ "noiseless");
  iqsdk::FullStateSimulator iqs_device(iqs_config);
  if (iqsdk::QRT_ERROR_SUCCESS != iqs_device.ready()) {
    return 1;
  }

  int num_shots = 10;
  std::cout << "Calling Deutsch-Jozsa on constant0 function (x" << num_shots
            << ")\n";
  for (int i = 0; i < num_shots; i++)
    deutschJozsa(&constant0);

  std::cout << "\nCalling Deutsch-Jozsa on constant1 function (x" << num_shots
            << ")\n";
  for (int i = 0; i < num_shots; i++)
    deutschJozsa(&constant1);

  std::cout << "\nCalling Deutsch-Jozsa on balanced function (b0,...) |-> b0 (x"
            << num_shots << ")\n";
  for (int i = 0; i < num_shots; i++)
    deutschJozsa(&balancedByQ0);

  std::cout << "\nCalling Deutsch-Jozsa on balanced function parity (x"
            << num_shots << ")\n";
  for (int i = 0; i < num_shots; i++)
    deutschJozsa(&balancedParity);
}
