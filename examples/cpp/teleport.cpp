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
// Quantum teleportation
//
// Quantum teleportation aims to send a quantum state from Alice to Bob using
// only classical communication. Alice and Bob start off each having half of a
// shared Bell pair, and by the end of the protocol, Bob's qubit is in a state
// corresponding to Alice's original qubit.
//
// The protocol uses three qubits:
//   1. alice_state is Alice's initial state
//   2. bell_a is Alice's half of the Bell pair
//   3. bell_b is Bob's half of the Bell pair
//
// The protocol proceeds in four steps:
// 1. [alice_prep_state] In the initialization step, Alice initializes her
//    qubit's state to the state |phi>.
// 2. [alice_protocol] Alice creates a bell pair entangles her qubit
//    (alice_state) with her half of the bell pair (bell_a).
// 3. [meas_alice] Alice measures her two qubits and sends the classical results
//    to Bob.
// 4. [qk_X_bob, qk_Z_bob] Bob corrects his state by applying either an X and/or
//    Z gate based on those results. Bob's qubit will now be in state |phi>, up
//    to a global phase.
//===----------------------------------------------------------------------===//
//
//
// This implementation highlights the usage of QRT state reporting using the API
// methods getProbabilities and getAmplitudes.
//
//
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

/// Quantum Runtime Library APIs
#include <quantum_full_state_simulator_backend.h>

#include <iostream>

/* Declare 3 qubits */
qbit alice_state;
qbit bell_a;
qbit bell_b;

/* Declare 3 measurement readouts */
cbit alice_bit1;
cbit alice_bit2;
cbit bob_result;

// Create a bell pair
quantum_kernel void prep_bell_pair(qbit& q0, qbit& q1) {
  PrepZ(q0);
  PrepZ(q1);
  H(q0);
  CNOT(q0, q1);
}

quantum_kernel void alice_prep_state() {
  // Prep all
  PrepZ(alice_state);
  PrepZ(bell_a);
  PrepZ(bell_b);

  // Alice creates her quantum state--here an X rotation
  RX(alice_state, 1);
}

quantum_kernel void alice_protocol() {
  // Create a bell pair
  prep_bell_pair(bell_a, bell_b);

  // Alice entangles her qubit with the first element of the Bell pair
  CNOT(alice_state, bell_a);
  H(alice_state);
}

// Alice measures her two bits and sends them to Bob.
quantum_kernel void meas_alice() {
  MeasZ(alice_state, alice_bit1);
  MeasZ(bell_a, alice_bit2);
}

// Corrections are invoked from the top level, so they must be wrapped in
// individual quantum_kernels
quantum_kernel void qk_X_bob() { X(bell_b); }
quantum_kernel void qk_Z_bob() { Z(bell_b); }

// Alternatively, push the corrections into a quantum kernel with CX and CZ
// operations.
quantum_kernel void bob_corrections() {
  CNOT(bell_a, bell_b);
  CZ(alice_state, bell_b);
}
quantum_kernel void teleport_circuit() {
  alice_prep_state();
  alice_protocol();
  meas_alice();
  bob_corrections();
}

int main() {
  using namespace iqsdk;

  // clang-format off
  /*
                      alice_prep_state()      alice_protocol()         meas_alice()     corrections
                    ===================    =====================     =============    =============
    |alice_state> : -- PrepZ --- RX -------------------- o --- H -------- MeasZ ----------------- o ----
                                                         |                                        |
    |bell_a>      : -- PrepZ --------------- H --- o --- X -------------- MeasZ ------- o ------- | ----
                                                   |                                    |         |
    |bell_b>      : -- PrepZ --------------------- X ---------------------------------- X ------- Z ----
  */
  // clang-format off

  // Set up the simulator
  IqsConfig iqs_config(/*num_qubits*/ 3,
                        /*simulation_type*/ "noiseless");
  FullStateSimulator iqs_device(iqs_config);
  if (QRT_ERROR_SUCCESS != iqs_device.ready()) {
    std::printf("Device not ready\n");
    return 1;
  }

  // Structures for reporting probabilities and amplitudes.
  std::vector<std::reference_wrapper<qbit>> qids = {
      std::ref(alice_state), std::ref(bell_a), std::ref(bell_b)};
  QssMap<double> probability_map;
  QssMap<std::complex<double>> amplitude_map;

  // Alice preps her state
  alice_prep_state();

  std::cout << "\n==Alice's state before teleportation==\n";
  // Only display non-zero amplitudes above 0.01
  amplitude_map = iqs_device.getAmplitudes(qids, {}, 0.01);
  FullStateSimulator::displayAmplitudes(amplitude_map);

  std::cout << "\n==Alice's corresponding probability distribution==\n";
  std::vector<std::reference_wrapper<qbit>> alice_qid_ref = {
      std::ref(alice_state)};
  // By specifying only alice_state for the qid argument, we get out the
  // conditional probability of alice's state being in |0> or |1> respectively
  probability_map = iqs_device.getProbabilities(alice_qid_ref, {});
  FullStateSimulator::displayProbabilities(probability_map);

  alice_protocol();

  // Before measurement, qubits are in superposition
  std::cout << "\n==Alice's state before meas_alice()==\n";
  // Only display non-zero amplitudes above 0.01
  amplitude_map = iqs_device.getAmplitudes(qids, {}, 0.01);
  FullStateSimulator::displayAmplitudes(amplitude_map);

  meas_alice();

  std::cout << "\n==Bob's state immediately after measurement==\n";
  // When we look at the state vector, we see that Alice's two qubits
  // have in fact been measured, and have collapsed so there are only two
  // non-zero amplitudes.
  amplitude_map = iqs_device.getAmplitudes(
      qids, {}, 0.01 /*threshold--only display non-zero amplitudes*/);
  FullStateSimulator::displayAmplitudes(amplitude_map);

  // If we look at the conditional probability associated with bob's qubit
  // (bell_b), we see that it now has the almost same distribution as Alice's
  // original prepared state, but with a possibility of the result being
  // swapped.
  std::cout << "\n==Bob's corresponding probability distribution==\n";
  std::vector<std::reference_wrapper<qbit>> bob_qid_ref = {std::ref(bell_b)};
  probability_map = iqs_device.getProbabilities(bob_qid_ref, {});
  FullStateSimulator::displayProbabilities(probability_map);

  // Now we apply corrections, using the results of alice's measurement to
  // decide on which corrections to apply.
  if (alice_bit2) {
    std::cout << "Applying X\n";
    qk_X_bob();
  }
  if (alice_bit1) {
    std::cout << "Applying Z\n";
    qk_Z_bob();
  }

  // After applying these corrections, we can project out the relevant
  // amplitudes and should get the same result as Alice's original prepared
  // state, up to a global phase
  std::cout << "\n==Bob's state after corrections have been applied==\n";
  // `bases` will expand into |alice_bit1 alice_bit2 0> and |alice_bit1
  // alice_bit2 1>
  std::vector<int> pattern = {alice_bit1, alice_bit2, QssIndex::Wildcard};
  std::vector<QssIndex> bases = QssIndex::patternToIndices(pattern);
  amplitude_map = iqs_device.getAmplitudes(qids, bases);
  FullStateSimulator::displayAmplitudes(amplitude_map);

  // The resulting probability distribution should be equivalent to Alice's
  // initial probability distribution
  std::cout << "\n==Bob's corresponding probability distribution==\n";
  probability_map = iqs_device.getProbabilities(bob_qid_ref, {});
  FullStateSimulator::displayProbabilities(probability_map);

  return 0;
}
