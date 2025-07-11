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
// MBL
// A repeatedly run experiment consisting of a single Trotter step of a
// 3-qubit version of the Many Body Localization (MBL) Algorithm. This is an
// extension to the example in
// quantum-examples/canonical_gate_variants/mbl_q3_1ts.cpp
// By changing the input angles, we are evaluating many different, but related
// Hamiltonians.
//
// References:
//
// Theory:
//
// 1. S. Johri, R. Nandkishore, and R. N. Bhatt, "Many-body localization in
// imperfectly isolated quantum systems," Phys. Rev. Lett., vol. 114, Mar. 2015,
// Art. no. 117401, doi: 10.1103/PhysRevLett.114.117401.
//
// 2. S. D. Geraedts, R. Nandkishore, and N. Regnault, "Many-body localization
// and thermalization: Insights from the entanglement spectrum," Phys. Rev. B,
// vol. 93, May 2016, Art. no. 174202, doi: 10.1103/PhysRevB.93.174202.
//
// Architectural study:
//
// 1. X. Zou et al., "Enhancing a Near-Term Quantum Accelerator's Instruction
// Set Architecture for Materials Science Applications," in IEEE Transactions on
// Quantum Engineering, vol. 1, pp. 1-7, 2020, Art no. 4500307,
// doi: 10.1109/TQE.2020.2965810.
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

/// Quantum Runtime Library APIs
#include <quantum_full_state_simulator_backend.h>

#include <iostream>
#include <random>

const int N = 3;
qbit Q[N];
cbit C[N];

double TsSeeds[6];

quantum_kernel void mblQ3_1ts() {
  for (int i = 0; i < N; i++) {
    PrepZ(Q[i]);
  }

  X(Q[0]);
  X(Q[2]);

  CNOT(Q[0], Q[1]);
  RZ(Q[0], TsSeeds[0]);
  RZ(Q[1], 8.0);
  H(Q[0]);
  CNOT(Q[0], Q[1]);
  RZ(Q[0], 8.0);
  RZ(Q[1], -8.0);
  CNOT(Q[0], Q[1]);
  H(Q[0]);
  CNOT(Q[0], Q[1]);

  RZ(Q[2], TsSeeds[1]);
  CNOT(Q[1], Q[2]);
  RZ(Q[1], TsSeeds[2]);
  RZ(Q[2], 8.0);
  H(Q[1]);
  CNOT(Q[1], Q[2]);
  RZ(Q[1], 8.0);
  RZ(Q[2], -8.0);
  CNOT(Q[1], Q[2]);
  H(Q[1]);
  CNOT(Q[1], Q[2]);

  RX(Q[0], TsSeeds[3]);
  RX(Q[1], TsSeeds[4]);
  RX(Q[2], TsSeeds[5]);

  for (int i = 0; i < N; i++) {
    MeasZ(Q[i], C[i]);
  }
}

int main() {
  /// Setup quantum device
  iqsdk::IqsConfig iqs_config(/*num_qubits*/ N,
                              /*simulation_type*/ "noiseless");
  iqsdk::FullStateSimulator iqs_device(iqs_config);
  if (iqsdk::QRT_ERROR_SUCCESS != iqs_device.ready()) {
    return 1;
  }

  double w = 6.0;

  int samples = 100;
  unsigned c_stat = 0;

  // respectable method of getting uniform random number
  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(-1.0, 1.0);

  for (int i = 0; i < samples; i++) {
    double hz0 = distribution(generator);
    TsSeeds[0] = static_cast<double>(8 * w * hz0);

    double hz2 = distribution(generator);
    TsSeeds[1] = static_cast<double>(8 * w * hz2);

    double hz1 = distribution(generator);
    TsSeeds[2] = static_cast<double>(8 * w * hz1);

    double hx0 = distribution(generator);
    TsSeeds[3] = static_cast<double>(8 * w * hx0);

    double hx1 = distribution(generator);
    TsSeeds[4] = static_cast<double>(8 * w * hx1);

    double hx2 = distribution(generator);
    TsSeeds[5] = static_cast<double>(8 * w * hx2);

    mblQ3_1ts();

    if (C[0] && !C[1] && C[2]) {
      c_stat++;
    }
  }

  std::cout << "Percentage of simulations with |101> "
            << static_cast<double>(100 * c_stat / samples) << "%\n";

  return 0;
}
