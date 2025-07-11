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
//
// Application: small Quantum Neural Network (QNN) composed by N-1 neurons.
//
//===----------------------------------------------------------------------===//

/// For each neuron (apart from those in the input layet) we use the construction
/// from [Cao, Guerreschi, & Aspuru-Guzik, arXiv:1711.11240]
///
/// The circuit for a neuron update based on Repeat Until Success (RUS) circuits.
///
/// in1:   |i> -----*-------------------------------------------------------*-------
///                 |                                                       |
/// in2:   |j> -----|--------*------------------------------------*---------|-------
///                 |        |                                    |         |
/// anc:   |0> ---Rx(w1)---Rx(w2)---Rx(b)---*---S^-1---Rx(-b)---Rx(-w2)---Rx(-w1)--- D~  -->  |0>:success    |1>:correct & repeat
///                                         |
/// out:   |0> -----------------------------X---------------------------------------          if successful --> Rx(g(b+i*w1+j*w2))|0>
///
/// The outcome of the ancilla's measurement determine whether the update was
/// successful or not. If it corresponds to |1>, then the update was not successful.
/// At this point one needs to undo the failed update and try again.
///
/// The recovery circuit involves only the ancilla and output qubits.
///
/// anc:   ----X---
///
/// out:   ---√X---
///
/// The successful update is confirmed by the ancilla measurement returning |1>.
/// In this case, the output qubit is rotated by an angle which is a non-linear
/// function of the input values.
/// Specifically one has:
///
///   g(y) = 2 arctan( tan(y/2)^2 )
///
/// where y = b + i*w1 + j*w2

/// The QNN implements a binary classifier in the supervised-learning approach.
/// The overall QNN is:
///
///   Q0:in1
///           \
///            > Q3:out
///           /
///   Q1:in2
///
///   Q4:anc
///   Q5:exp   <-- expected output
///                (it may be played by the ancilla if posponed until after
///                 the QNN inference is run)
///
/// NOTE: This code assumes that we have a total of NQ=N+3 qubits including:
///         input, hidden, output, ancilla, expected_output
///       The N input qubits are the first ones (starting with index 0),
///       then follows the output qubit, ancilla, and finally expected output
///       (the last one with index NQ-1=N+2).

#include <iostream>
#include <math.h>
#include <random>

#include <clang/Quantum/quintrinsics.h>

#include <quantum_full_state_simulator_backend.h>

// Global register of qubits
const int N = 2;
const int NQ = N+1+1+1;

const int id_out = N;
const int id_anc = N+1;
const int id_exp = N+2;

qbit qubit_reg[NQ];
cbit cbit_reg[NQ];

// Individual quantum kernels, i.e. blocks of quantum operations (prep, gates, meas)

/// Quantum kernel for state preparation:
/// The input states are in the superposition of all bitstrings.
quantum_kernel void Initialization() {
  // Initialization of all qubits.
  for (int index = 0; index < NQ; index++)
    PrepZ(qubit_reg[index]);
  for (int index = 0; index < N; index++)
    H(qubit_reg[index]);
}

/// Oracle playing the role of the function to be learned.
/// For this example, we consider the XOR function.
///   in1   in2    out
///  ------------------
///   |0>   |0>    |0>
///   |0>   |1>    |1>
///   |1>   |0>    |1>
///   |1>   |1>    |0>
/// The expected output is prepared in the ancilla qubit.
quantum_kernel void OracleFunction() {
  PrepZ(qubit_reg[id_exp]);
  for (int j=0; j<N; ++j) {
      CNOT(qubit_reg[j], qubit_reg[id_exp]);
  }
}

quantum_kernel void RUSCircuit(double params[]) {
  for (int j=0; j<N; ++j) {
    // Controlled-RX (q[inp[j]], q[anc],  weight[j]);
    CZ(qubit_reg[j], qubit_reg[id_anc]);
    RX(qubit_reg[id_anc], -params[j]/2);
    CZ(qubit_reg[j], qubit_reg[id_anc]);
    RX(qubit_reg[id_anc],  params[j]/2);
  }
  RX(qubit_reg[id_anc], params[N]);
  CNOT(qubit_reg[id_anc], qubit_reg[id_out]);
  Sdag(qubit_reg[id_anc]);
  RX(qubit_reg[id_anc], -params[N]);
  for (int j=0; j<N; ++j) {
    // Controlled-RX (q[inp[j]], q[anc], -weight[j]);
    RX(qubit_reg[id_anc], -params[j]/2);
    CZ(qubit_reg[j], qubit_reg[id_anc]);
    RX(qubit_reg[id_anc],  params[j]/2);
    CZ(qubit_reg[j], qubit_reg[id_anc]);
  }
}

// Measuring ancilla qubit.
quantum_kernel void MeasAncilla() {
  MeasZ(qubit_reg[id_anc], cbit_reg[id_anc]);
}

// Recovery after a failed neuron update.
quantum_kernel void Recovery() {
  X(qubit_reg[id_anc]);
  RX(qubit_reg[id_out], M_PI/2);
}

/// Update of the output neuron.
void NeuronUpdate(double params[]) {
  bool was_successful = false;
  while (was_successful != true) {
    RUSCircuit(params);
    // Measure ancilla to determine whether repeating.
    MeasAncilla();
    if (cbit_reg[id_anc] == 1)
      Recovery();
    else
      was_successful = true;
  }
}

/// Measure the QNN accuracy.
///
/// It evaluates the parity between the output qubit and the expected output.
/// The even parity corresponds to |0>, so we also apply X to flip the state.
/// By measuring <Z> on the 'parity qubit' one can quantify the QNN fitness.
quantum_kernel void MeasNetworkAccuracy() {
  CNOT(qubit_reg[id_out], qubit_reg[id_exp]);
  MeasZ(qubit_reg[id_exp], cbit_reg[id_exp]);
}

//===----------------------------------------------------------------------===//

/// Main program calling multiple quantum kernels
int main() {

  // By default, the IQS backend is noiseless.
  iqsdk::IqsConfig iqs_config;
  iqsdk::FullStateSimulator iqs_device(iqs_config);
  iqsdk::QRT_ERROR_T status_iqs = iqs_device.ready();

  // Initialize all bias and weights at random.
  double params[N+1];
  std::size_t seed = 7777;
  std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded.
  std::uniform_real_distribution<> dist(0, M_PI);
  for (int n=0; n<N+1; ++n)
      params[n] = dist(gen);

  std::cout << "Output neuron:\n"
            << "  b = " << params[N] << "\n";
  for (int n=0; n<N; ++n)
      std::cout << "  w[" << n << "] = " << params[n] << "\n";
  unsigned num_runs = 100;
  unsigned counter = 0;

  for (unsigned r=0; r<num_runs; ++r) {
      Initialization();
      OracleFunction();
      NeuronUpdate(params);
      MeasNetworkAccuracy();
      counter += (unsigned)cbit_reg[id_exp];
  }

  std::cout << "\nOut of " << num_runs << " runs, we measured the parity between \n"
            << "expected output and QNN output to be odd for a total of " << counter << " times.\n"
            << "This corresponds to a cost value of " << (double)counter/num_runs << "\n";
  return 0;
}
