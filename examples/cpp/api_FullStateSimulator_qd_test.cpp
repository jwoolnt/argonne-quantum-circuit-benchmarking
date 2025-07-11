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

#include <clang/Quantum/quintrinsics.h>

/// Quantum Runtime Library APIs
#include <quantum_full_state_simulator_backend.h>

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
  iqsdk::DeviceConfig qd_config("QD_SIM");
  iqsdk::FullStateSimulator quantum_8086;
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
  quantum_8086.initialize(qd_config);
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

  std::cout << std::endl << "testing . . .  getSingleQubitProbs" << std::endl;
  std::vector<double> prob_vec = quantum_8086.getSingleQubitProbs(qids);
  for (auto const &entry : prob_vec) {
    std::cout << entry << std::endl;
  }

  // use string constructor of Quantum State Space index to choose which
  // basis states' data is retrieved
  std::vector<bool> zero;
  for (int i = 0; i < total_qubits; ++i) {
    zero.push_back(false);
  }

  std::vector<bool> one;
  for (int i = 0; i < total_qubits; ++i) {
    one.push_back(true);
  }

  std::vector<iqsdk::QssIndex> bases = {iqsdk::QssIndex(zero),
                                        iqsdk::QssIndex(one)};

  std::cout << std::endl << "testing . . .  getProbabilities" << std::endl;
  iqsdk::QssMap<double> probability_map;
  probability_map = quantum_8086.getProbabilities(qids, bases);
  double total_probability = 0.0;
  for (auto const &key_value : probability_map) {
    total_probability += key_value.second;
  }
  std::cout << "Sum of probability to measure fully entangled state: "
            << total_probability << std::endl;

  std::cout << std::endl << "testing . . .  displayProbabilities" << std::endl;
  quantum_8086.displayProbabilities(probability_map);

  std::cout << std::endl << "testing . . .  getSamples" << std::endl;
  std::vector<std::vector<bool>> measurement_samples;
  unsigned total_samples = 10;
  measurement_samples = quantum_8086.getSamples(total_samples, qids);
  for (auto measurement : measurement_samples) {
    for (auto result : measurement) {
      std::cout << result;
    }
    std::cout << std::endl;
  }

  std::cout << std::endl << "testing . . .  samplesToHistogram" << std::endl;
  iqsdk::QssMap<unsigned int> distribution =
      iqsdk::FullStateSimulator::samplesToHistogram(measurement_samples);
  std::cout << "Using " << total_samples
            << " samples, the distribution of states is:" << std::endl;
  for (const auto &entry : distribution) {
    double weight = entry.second;
    weight = weight / total_samples;

    std::cout << entry.first << " : " << weight << std::endl;
  }

  return 0;
}
