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
// Application: small Quantum Neural Network (QNN) composed by N-N-1 neurons.
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
///   Q0:in1  -----  Q2:h11
///            \ /           \
///             X             > Q4:out
///            / \           /
///   Q1:in2  -----  Q3:h12
///
///   Q5:anc
///   Q6:exp   <-- expected output
///                (it may be played by the ancilla if posponed until after
///                 the QNN inference is run)
///
/// NOTE: This code assumes that we have a total of K=2*N+3 qubits including:
///         input, hidden, output, ancilla, expected_output
///       The n input qubits are the first ones (starting with index 0),
///       then follows the n hidden qubits (starting with index n), then
//        output, ancilla, and finally expected output (the last one with
//        index K-1=2*N+2).


#include <iostream>
#include <math.h>
#include <random>

#include <clang/Quantum/quintrinsics.h>

#include <quantum_full_state_simulator_backend.h>

// Global register of qubits
const int N = 2;
const int num_anc = 1;
const int K = N + N + 1 + num_anc + 1;
qbit q[K];
cbit c[K];

const int id_inp = 0;             // First input neuron
const int id_hid = N;             // First hidden neuron
const int id_out = 2*N;           // QNN output neuron
const int id_anc = 2*N+1;         // First ancilla qubit
const int id_exp = 2*N+1+num_anc; // Expected output

// Individual quantum kernels, i.e. blocks of quantum operations (prep, gates, meas)

/// Quantum kernel for state preparation:
/// The input states are in the superposition of all bitstrings.
quantum_kernel void input_preparation() {
  for (int j=id_inp; j<id_hid; ++j) {
      PrepZ(q[j]);
      H(q[j]);
      //X(q[j]);
  }
  for (int j=id_hid; j<K; ++j)
      PrepZ(q[j]);
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
quantum_kernel void expected_output_preparation() {
  PrepZ(q[id_exp]);
  for (int j=id_inp; j<id_hid; ++j) {
      CNOT(q[j], q[id_exp]);
  }
}

/// Update of a neuron.
///
/// Template interpretation:
///  out -> index of the qubit corresponding to the neuron to  update
///  inp -> first index of the input qubits (there are num_inp of those)
template<int out, int inp>
quantum_kernel void update_neuron(double bias, double weight[]) {
  for (int j=0; j<N; ++j) {
    // Controlled-RX (q[inp[j]], q[anc],  weight[j]);
    CZ(q[inp+j], q[id_anc]);
    RX(q[id_anc], -weight[j]/2);
    CZ(q[inp+j], q[id_anc]);
    RX(q[id_anc],  weight[j]/2);
  }
  RX(q[id_anc], bias);

  CNOT(q[id_anc], q[out]);
  Sdag(q[id_anc]);

  RX(q[id_anc], -bias);
  for (int j=0; j<N; ++j) {
    // Controlled-RX (q[inp[j]], q[anc], -weight[j]);
    RX(q[id_anc], -weight[j]/2);
    CZ(q[inp+j], q[id_anc]);
    RX(q[id_anc],  weight[j]/2);
    CZ(q[inp+j], q[id_anc]);
  }
}

/// Recovery after a failed neuron update.
///
/// Template interpretation:
///  out -> index of the qubit corresponding to the neuron to  update
template<int out>
quantum_kernel void recovery() {
  X(q[id_anc]);
  RX(q[out], M_PI/2);
}

/// Quantum kernel for measuring the ancilla qubit.
quantum_kernel void ancilla_measurement() {
  MeasZ(q[id_anc], c[id_anc]);
}

/// Measure the QNN fitness.
///
/// It evaluates the parity between the output qubit and the expected output.
/// The even parity corresponds to |0>, so we also apply X to flip the state.
/// By measuring <Z> on the 'parity qubit' one can quantify the QNN fitness.
quantum_kernel void fitness_measurement() {
  CNOT(q[id_exp], q[id_out]);
  MeasZ(q[id_out], c[id_out]);
}

//===----------------------------------------------------------------------===//
// Instantiate the quantum kernels using recursive functions.

/// RUS technique for neuron update.
///
/// Template interpretation:
///  j -> index of the qubit corresponding to the neuron (of the hidden layer) to update
template <int j>
void update_hidden_neurons_loop(double* bias, double* weight) {
  int pos = j-N; // Position in the parameter array
  double b = bias[pos];
  double w[N];
  for (unsigned i=0; i<N; ++i)
    w[i] = weight[pos*N + i];

  bool was_succ = false;
  while (was_succ != true) {
    update_neuron<j, 0>(b, w);
    ancilla_measurement();
    if (c[id_anc] == 1)
        recovery<j>();
    else
        was_succ = true;
    }
}

template <int J>
void call_update_hidden_neurons_loop(double* bias, double* weight, typename std::enable_if<(J >= 2*N)>::type * = 0) {}

template <int J>
void call_update_hidden_neurons_loop(double* bias, double* weight, typename std::enable_if<(J < 2*N)>::type * = 0) {
    update_hidden_neurons_loop<J>(bias, weight);
    call_update_hidden_neurons_loop<J+1>(bias, weight);
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

/// Main program calling multiple quantum kernels
int main() {

  // By default, the IQS backend is noiseless.
  iqsdk::IqsConfig iqs_config;
  iqsdk::FullStateSimulator iqs_device(iqs_config);
  iqsdk::QRT_ERROR_T status_iqs = iqs_device.ready();

#if 10
  double bias[N+1];
  double weight[N*(N+1)];
  // Initialize all bias and weights at random.
  std::size_t seed = 7777;
  std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded.
  std::uniform_real_distribution<> dist(0, M_PI);
  for (int n=0; n<N+1; ++n) { // <-- neuron to update has index N+n
      bias[n] = dist(gen);
      for (int m=0; m<N; ++m) // <-- m is the relative index of the N inputs
          weight[N*n+m] = dist(gen);
  }
#else
  // Debug: Initialize bias and weights with specific values.
  //   hid[0] = ~inp[0]
  //   hid[1] =  inp[1]
  //   out    = ~(hid[0] + hid[1])
  double bias[N+1] = {M_PI, 0, M_PI};
  double weight[N*(N+1)] = {M_PI, 0,    0, M_PI,     M_PI, M_PI};
  //double weight[N*(N+1)];
  //for (int i=0; i<N*(N+1); ++i)
  //    weight[i] = 0;
#endif
  for (int n=0; n<N+1; ++n) { // <-- neuron to update has index N+n
      std::cout << "Hidden neuron q[" << N+n << "]:\n"
                << "  b = " << bias[n] << "\n"
                << "  w = ";
      for (int m=0; m<N; ++m) // <-- m is the relative index of the N inputs
          std::cout << weight[N*n+m] << " , ";
      std::cout << "\n";
  }
  unsigned num_runs = 100;
  unsigned counter = 0;

  for (unsigned r=0; r<num_runs; ++r) {
      input_preparation();
      expected_output_preparation();

      // Update neurons in the hidden layer.
      call_update_hidden_neurons_loop<id_hid>(bias, weight);

      // Update output neuron.
      double b = bias[N];
      double w[N];
      for (unsigned j=0; j<N; ++j)
          w[j] = weight[N*N+j];
      bool was_succ = false;
      while (was_succ != true) {
          update_neuron<id_out, id_hid>(b, w);
          ancilla_measurement();
          if (c[id_anc] == 1)
              recovery<id_out>();
          else
              was_succ = true;
      }

      // Measure network fitness.
      fitness_measurement();
      std::cout << "[run " << r << "] Parity = " << (unsigned)c[id_out] << "\n";
      counter += (unsigned)c[id_out];
  }

  std::cout << "\nOut of " << num_runs << " runs, we measured the parity between \n"
            << "expected output and QNN output to be odd for a total of " << counter << " times.\n"
            << "This corresponds to a cost value of " << (double)counter/num_runs << "\n";
  return 0;
}
