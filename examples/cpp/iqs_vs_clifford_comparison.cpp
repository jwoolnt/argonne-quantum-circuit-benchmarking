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
// A demonstration of a comparison of generating a simple entangled state on the
// Intel Quantum Simulator vs. Clifford Simulator. Also illustrates parallel
// kickoff and results collection of multiple Clifford simulator instances.
//
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>
#include <quantum.hpp>
#include <quantum_clifford_simulator_backend.h>

#include <bits/stdc++.h>
#include <cstdlib>

// number of samples to consider for the comparison. higher number should give a
// better average.
constexpr size_t clifford_samples = 1000;
const int total_qubits = 10;
// This is the number of non-trivial states we expect to be populated from the
// quantum kernel
constexpr size_t non_trivial_state_num = 8;

qbit qubit_reg[total_qubits];

// global multi-dimensional cbit register to hold samples from each run on a
// Clifford simulator instance
cbit cbit_reg[clifford_samples][total_qubits];

// Quantum kernel to be run with IQS. Doesn't include the MeasZ commands to
// be able to extract the probabilities directly.
quantum_kernel void entangledStateIqs() {
  for (int i = 0; i < total_qubits; i++) {
    PrepZ(qubit_reg[i]);
  }

  RY(qubit_reg[0], -M_PI_2);
  RX(qubit_reg[9], M_PI_2);
  RY(qubit_reg[3], -M_PI_2);

  for (int i = 0; i < total_qubits - 1; i++) {
    CNOT(qubit_reg[i], qubit_reg[i + 1]);
  }
}

// Quantum kernel to be run with Clifford Simulator. Includes the MeasZ
// commands to enable sampling of cbits.
quantum_kernel void entangledStateClifford(int run_idx) {
  entangledStateIqs();

  for (int i = 0; i < total_qubits; i++) {
    MeasZ(qubit_reg[i], cbit_reg[run_idx][i]);
  }
}

// Function signatures for running on the two different simulators
iqsdk::QssMap<double>
runIqsSimulation(std::vector<std::reference_wrapper<qbit>> qids);
void runCliffordSimulation(std::vector<std::reference_wrapper<qbit>> qids);

int main() {

  // Initializing the random number generation
  srand((unsigned)time(NULL));

  std::vector<std::reference_wrapper<qbit>> qids;
  for (int id = 0; id < total_qubits; ++id) {
    qids.push_back(std::ref(qubit_reg[id]));
  }

  // Run full IQS simulation and return the relevant results
  iqsdk::QssMap<double> thresholded_prob_map = runIqsSimulation(qids);

  std::vector<std::vector<bool>> non_trivial_states;
  std::vector<double> non_trivial_probabilities;

  // organizing the thresholded results from IQS for later comparison with the
  // Clifford Simulator
  for (auto &it : thresholded_prob_map) {
    std::vector<bool> fullState = (it.first).getBasis();
    double expected_probability = it.second;

    non_trivial_states.push_back(fullState);
    non_trivial_probabilities.push_back(expected_probability);
  }

  runCliffordSimulation(qids);

  // single shot instance count corresponding to each non trivial state
  int single_shot_instances[non_trivial_state_num] = {0};
  // probability from Clifford sim corresponding to each non trivial state
  double probability_instances[non_trivial_state_num];

  for (size_t itr = 0; itr < clifford_samples; itr++) {
    std::vector<bool> current_state;

    for (int i = 0; i < total_qubits; i++) {
      current_state.push_back(cbit_reg[itr][i]);
    }

    // perform comparison of the state bit sting from the Clifford simulation
    // vs. IQS, and aggregate accordingly
    for (int state_idx = 0; state_idx < non_trivial_state_num; state_idx++) {
      if (current_state == non_trivial_states[state_idx]) {
        single_shot_instances[state_idx]++;
      }
    }

    // calculate actual probabilities for the Clifford Simulator results
    for (int state_idx = 0; state_idx < non_trivial_state_num; state_idx++) {
      probability_instances[state_idx] =
          ((double)single_shot_instances[state_idx]) / clifford_samples;
    }
  }

  std::cout << "Inaccuracy evaluation of calculated probabilities from "
               "CliffordSim vs. exact probabilities from IQS"
            << std::endl;

  // calculate and note the percentage difference of the state probabilities
  // derived from Clifford simulation samples vs. the Intel Quantum Simulator
  for (int state_idx = 0; state_idx < non_trivial_state_num; state_idx++) {
    double inaccuracy = std::abs(probability_instances[state_idx] -
                                 non_trivial_probabilities[state_idx]) /
                        non_trivial_probabilities[state_idx];

    std::cout << "Non-trivial state comparison #" << state_idx << " = "
              << inaccuracy * 100 << "% (" << probability_instances[state_idx]
              << ", " << non_trivial_probabilities[state_idx] << ")"
              << std::endl;
  }

  return 0;
}

// perform a single simulation with the Intel Quantum Simulator and directly
// extract the state probabilities without performing actual measurements
iqsdk::QssMap<double>
runIqsSimulation(std::vector<std::reference_wrapper<qbit>> qids) {

  iqsdk::FullStateSimulator iqs_sim;
  iqsdk::IqsConfig iqs_config(total_qubits);
  iqs_config.verbose = false;
  iqs_sim.initialize(iqs_config);
  iqs_sim.ready();

  entangledStateIqs();

  std::vector<iqsdk::QssIndex> bases;
  iqsdk::QssMap<double> probability_map;

  probability_map = iqs_sim.getProbabilities(qids, bases, 0.1);
  iqs_sim.displayProbabilities(probability_map);

  return probability_map;
}

// perform multiple Clifford simulations (each initialized with a random seed)
// and collect samples
void runCliffordSimulation(std::vector<std::reference_wrapper<qbit>> qids) {
  iqsdk::CliffordSimulator cliff_sim[clifford_samples];

  for (size_t i = 0; i < clifford_samples; i++) {
    iqsdk::CliffordSimulatorConfig cliff_config(rand());
    cliff_config.synchronous = false;
    cliff_config.verbose = false;

    cliff_sim[i].initialize(cliff_config);
  }

  for (size_t itr = 0; itr < clifford_samples; itr++) {
    cliff_sim[itr].ready();
    entangledStateClifford(itr);
  }

  for (size_t itr = 0; itr < clifford_samples; itr++) {
    cliff_sim[itr].wait();
  }
}
