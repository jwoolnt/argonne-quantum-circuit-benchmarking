//===----------------------------------------------------------------------===//
// Copyright 2023-2024 Intel Corporation.
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
// Program to simulate the Quantum Key Distribution BB84 Algorithm
//
// inspired by the work published in:
//
// C.H. Bennett, G. Brassard
// Quantum cryptography: public key distribution and coin tossing
// Theoretical Aspects of Quantum Cryptography, Celebrating 30 Years of BB84,
// Theor. Comput. Sci., 560 (2014), pp. 7-11
// https://doi.org/10.1016/j.tcs.2014.05.025
//
// The purpose of quantum key distribution is to create an encryption key
// known to only two people.
//
// In this simulation, Alice tries to create a key and to send it to Bob
// encoded as qubits. There is a chance that a third-party, Emory, will try to

// listen to the key exchange; but the protocol allows Alice and Bob to avoid
// having Emory learn the key.
//
// First, Alice creates two sequences of random bits. One sequence is a set of
// candidate values to make up a key, and the other set indicates which basis
// Alice will use to encode the qubit's state.
//
// Once encoded, Alice sends the state of the prepared qubits to Bob. Bob
// decides which basis to use for measurement randomly and records his
// observations. He communicates his basis choices to Alice, and she confirms
// the qubits where they chose the same basis.

//
// For these qubits, Alice and Bob should have identical observations (the qubit
// was prepared in an eigenstate and measured in that same basis).
// Bob and Alice can now compare Bob's observations with Alice's candidate bit
// for each of these qubits. If Alice and Bob observe different outcomes for any
// qubit where they share a basis, then they know someone has listened in on the
// quantum channel (made an observation of a qubit). Alice and Bob then abandon
// the channel and flee to their respective safe houses.
//
// Practically speaking, the set of qubits with matching bases can be divided
// into a subset that is checked for listeners and a subset that is the
// message, or more complex information reconciliation and privacy
// amplification procedures can be implemented. Here, Alice and Bob check the
// entire set of qubits.

#include <clang/Quantum/quintrinsics.h>
#include <quantum.hpp>

const int NUM_QUBITS = 24; // modifying impacts the security of both the key and
                           // detectability of listeners; try experimenting

const double pi = M_PI;

qbit quantum_buffer[NUM_QUBITS];
cbit bob_outcomes[NUM_QUBITS];
cbit emory_outcomes[NUM_QUBITS];

/* Use two input bits to prepare a qubit.
 * The first input bit encodes the basis from which to view the encoded bit's
 * state, 0 corresponds to the Z basis and 1 corresponds to the X basis.
 * The second input bit is the value to be encoded into the qubit.
 * The quantum_kernel prepares a rotation around the Y-axis with the angle
 * determined by the inputs.
 */
quantum_kernel void writeToQubit(const int basis, const int bit_state,
                                 qbit &qubit_to_encode) {
  double angle;
  if (basis == 0) {
    if (bit_state == 0) {
      angle = 0; // Computational 0
    } else {
      angle = pi; // Computational 1
    }
  } else {
    if (bit_state == 0) {
      angle = pi / 2; // Computational +
    } else {
      angle = 3 * pi / 2; // Computational -
    }
  }
  RY(qubit_to_encode, angle);
}

/* Use two sequences of bits to prepare the candidate key values into the
 * quantum buffer.
 * For each qubit in the quantum buffer, the quantum_kernel initializes
 * its state and writes the encoded bit to it.
 */
quantum_kernel void encodeQuantumKey(std::vector<int> bits,
                                     std::vector<int> bases) {
  for (int i = 0; i < NUM_QUBITS; i++) {
    PrepZ(quantum_buffer[i]);
    writeToQubit(bases[i], bits[i], quantum_buffer[i]);
  }
}

/* An input value indicates the basis Bob has randomly chosen to measure
 * against.
 * The quantum_kernel performs a rotation of the qubit into the measurement
 * basis that Bob chose.
 */
quantum_kernel void measurementBasis(const int bob_basis, qbit &this_qubit) {
  double angle;
  if (bob_basis == 1) { // If Bob's basis is 1; measure in the X basis
    angle = -pi / 2;
  } else {
    angle = 0;
  }
  RY(this_qubit, angle);
}

/* An input sequence specifies Bob's choices to measure each qubit.
 * The quantum_kernel performs any needed change of basis using measurementBasis
 * and then measures and records the outcome for each qubit.
 */
quantum_kernel void decodingBobRegister(std::vector<int> bases) {
  for (int i = 0; i < NUM_QUBITS; i++) {
    measurementBasis(bases[i], quantum_buffer[i]);
    MeasZ(quantum_buffer[i], bob_outcomes[i]);
  }
}

/* The listenIn() quantum_kernel simulates the act of eavesdropping (if an
 * attacker has been generated) by invoking measurements on the qubit.
 * This is the simplest implementation, and not representative of a best
 * attack.
 */
quantum_kernel void listenIn() {
  for (int i = 0; i < NUM_QUBITS; i++) {
    MeasZ(quantum_buffer[i], emory_outcomes[i]);
  }
}

int main() {
  using namespace iqsdk;

  // Set up the simulator that represent the quantum communication channel
  std::cout << "Alice boots her quantum communication system.\n";
  IqsConfig iqs_config(NUM_QUBITS);

  FullStateSimulator iqs_device(iqs_config);

  if (QRT_ERROR_SUCCESS != iqs_device.ready()) {
    std::printf("Device not ready\n");
    return 1;
  }

  // Alice creates a random selection of bits to transmit
  std::cout << "Alice uses a random number generator to create two bit "
            << "sequences.\n";
  std::vector<int> alice_bits;
  srand(time(0)); // seed the random number generator with current time;
                  // will produce different results every execution
  for (int i = 0; i < NUM_QUBITS; i++) {
    int bit = rand() % 2;
    alice_bits.push_back(bit);
  }

  // Next, Alice creates a random choice of quantum basis in which to encode
  // the previously generated bit.
  std::vector<int> alice_basis;
  for (int i = 0; i < NUM_QUBITS; i++) {
    int basis = rand() % 2;
    alice_basis.push_back(basis);
  }

  std::cout << "Alice privately reviews her choices.\n";
  std::cout << "Alice's candidate key sequence:  \n";
  for (const auto &bit : alice_bits) {
    std::cout << bit;
  }
  std::cout << "\n";
  std::cout << "Alice's bases sequence:  \n";
  for (const auto &bit : alice_basis) {
    std::cout << bit;
  }
  std::cout << "\n";

  // Alice then encodes the bits into the quantum_buffer
  std::cout << "Alice encodes and sends the pulse of qubits.\n\n";
  encodeQuantumKey(alice_bits, alice_basis);

  std::cout << "An omniscient observer would be able to see the pulse's "
            << "possible states that result from eavesdropping:  \n";
  // reference all qubits because all qubits are significant
  std::vector<std::reference_wrapper<qbit>> qids;
  for (int id = 0; id < NUM_QUBITS; ++id) {
    qids.push_back(std::ref(quantum_buffer[id]));
  }
  // build a wildcard string to create state space of correct size
  std::string wildcard;
  for (int j = 0; j < NUM_QUBITS; ++j) {
    wildcard.push_back('?');
  }
  std::vector<QssIndex> all_states = QssIndex::patternToIndices(wildcard);
  QssMap<double> encoded_states =
      iqs_device.getProbabilities(qids, all_states, 0.0000001);
  std::cout << "There are " << encoded_states.size()
            << " possible results (that the proposed eavesdropping attempt "
               "might observe).\n";
  if (encoded_states.size() == 1) {
    std::cout << "Uh oh, an eavesdropper may go undetected\n";
  } else if (encoded_states.size() < 20) {
    FullStateSimulator::displayProbabilities(encoded_states);
  }

  // Bob chooses which basis to use when measuring each qubit
  std::cout << "\nFor each qubit, Bob uses his random number generator to "
            << "decide which basis to use to measure.\n";
  std::vector<int> bob_bases;
  for (int i = 0; i < NUM_QUBITS; i++) {
    int basis = rand() % 2;
    bob_bases.push_back(basis);
  }

  int emory_strikes = rand() % 2;
  if (emory_strikes == 1) {
    std::cout << "\n** A wild Emory appears! **\n"
              << "** Evil Emory secretly listens! **\n\n";
    listenIn();

    std::cout << "The omniscient observer checks the quantum state of the "
              << "message again and finds:\n";
    encoded_states = iqs_device.getProbabilities(qids, all_states, 0.0000001);
    std::cout << "There are " << encoded_states.size() << " possible result.\n";
    FullStateSimulator::displayProbabilities(encoded_states);
  }

  // Bob decodes the contents of the quantum buffer according to his bases
  decodingBobRegister(bob_bases);

  // Bob sends his basis information to Alice
  // In principle, just a fraction of the set is sent through open channel.
  // but in this implementation, Bob announces his basis choice for all qubits.
  // Alice verifies to Bob which qubits their choices matched.
  std::cout << "\nBob sends his basis choices to Alice:\n"
            << "Bob's basis set:\n  ";
  for (const auto &bit : bob_bases) {
    std::cout << bit;
  }
  std::cout << "\n";
  std::vector<bool> matching_bases;
  for (int i = 0; i < alice_basis.size(); i++) {
    if (alice_basis.at(i) == bob_bases.at(i)) {
      matching_bases.push_back(true);
      std::cout << "Alice and Bob agree to use bit " << i << " in the key.\n";
    } else {
      matching_bases.push_back(false);
    }
  }

  // Converting Bob's Measurements from cbits into int
  std::vector<int> bob_measurements;
  for (const auto &outcome : bob_outcomes) {
    if (outcome) {
      bob_measurements.push_back(1);
    } else {
      bob_measurements.push_back(0);
    }
    // the above is written with conditionals for reading clarity, but using a
    // cast from cbit to int is equivalent and more efficient
    // bob_measurements.push_back((int)(outcomes)) // instead of if-else
  }

  std::cout << "Bob's Measurements:\n  ";
  for (const auto &measurement : bob_measurements) {
    std::cout << measurement;
  }
  std::cout << "\n";

  // Alice and Bob compare their exchanged bits (usually a fraction, here all)
  // and they check that a certain number of them agree (here all)
  bool emory_is_here = false; // assume there is no listener
  for (int i = 0; i < matching_bases.size(); i++) {
    if (matching_bases.at(i) == true) {
      if (alice_bits.at(i) != bob_measurements.at(i)) {
        emory_is_here = true;
        std::cout << "Alice and Bob disagree on bit " << i << "\n";
      } else { /* Do nothing */
      }
    } else { /* Do nothing */
    }
  }

  if (emory_is_here) {
    std::cout << "\nA man-in-the-middle attack!"
              << " Run, Alice and Bob! Run away!\n";
  } else {
    std::cout << "Alice and Bob will create an encryption key seeded with:\n";
    for (int i = 0; i < matching_bases.size(); i++) {
      if (matching_bases.at(i) == true) {
        std::cout << alice_bits.at(i);
      } else { /* Do nothing */
      }
    }
    std::cout << "\nand immediately begin to establish stronger encryption.\n";
  }

  return 0;
}
