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

#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <quantum_custom_backend.h>
#include <qureg.hpp>
#include <string.h>

// The purpose of this program is to showcase the use of a custom backend. In
// this example we use the Intel Quantum Simulator (IQS) as the underlying
// simulation engine, but access it as a custom backend.

const int N = 3;
qbit q[N];
cbit c[N];

quantum_kernel void mbl_q3_1ts() {

  for (int i = 0; i < N; i++) {
    PrepZ(q[i]);
  }

  X(q[0]);
  X(q[2]);

  CNOT(q[0], q[1]);
  RZ(q[0], 9.563581772879);
  RZ(q[1], 8.0);
  H(q[0]);
  CNOT(q[0], q[1]);
  RZ(q[0], 8.0);
  RZ(q[1], -8.0);
  CNOT(q[0], q[1]);
  H(q[0]);
  CNOT(q[0], q[1]);

  RZ(q[2], 1.415202001624);
  CNOT(q[1], q[2]);
  RZ(q[1], 7.070381830323);
  RZ(q[2], 8.0);
  H(q[1]);
  CNOT(q[1], q[2]);
  RZ(q[1], 8.0);
  RZ(q[2], -8.0);
  CNOT(q[1], q[2]);
  H(q[1]);
  CNOT(q[1], q[2]);

  RX(q[0], 4.94709917593);
  RX(q[1], 5.041840372001);
  RX(q[2], 2.56278001524);
}

class CustomBackend : public iqsdk::CustomInterface {
public:
  iqs::QubitRegister<ComplexDP> psi;
  iqs::RandomNumberGenerator<double> rng;
  CustomBackend(int num_qubits)
      : psi(iqs::QubitRegister<ComplexDP>(num_qubits, "base", 0)) {
    int seed = 0;
    rng.SetSeedStreamPtrs(seed);
  }

  void RXY(qbit q, double theta, double phi) {
    psi.ApplyRotationXY(q, theta, phi);
  }

  void RZ(qbit q, double angle) { psi.ApplyRotationZ(q, angle); }

  void CPhase(qbit ctrl, qbit target, double angle) {
    psi.ApplyCPhaseRotation(ctrl, target, -angle);
  }

  void SwapA(qbit q1, qbit q2, double angle) {
    iqs::TinyMatrix<ComplexDP, 2, 2, 32> gate_matrix;
    gate_matrix(0, 0) = gate_matrix(1, 1) = {0.5 * (1.0 + std::cos(angle)),
                                             0.5 * std::sin(angle)};
    gate_matrix(0, 1) = gate_matrix(1, 0) = {0.5 * (1.0 - std::cos(angle)),
                                             -0.5 * std::sin(angle)};
    psi.ApplyISwapRotation(q1, q2, gate_matrix);
  }

  void PrepZ(qbit q) {
    // Should do something here if the state is non-trivial.
    // For this example we are not doing any operations before putting
    // qubits in |0> state
  }

  cbit MeasZ(qbit q) {
    cbit measurement;
    double rand_value, probability;
    probability = psi.GetProbability(q);
    rng.UniformRandomNumbers(&rand_value, 1, 0, 1, "state");
    if (rand_value > probability) {
      measurement = false;
      psi.CollapseQubit(q, false);
    } else {
      measurement = true;
      psi.CollapseQubit(q, true);
    }
    psi.Normalize();
    return measurement;
  }
};

int main() {
  iqsdk::CustomSimulator *custom_simulator =
      iqsdk::CustomSimulator::createSimulator<CustomBackend>("my_custom_device",
                                                             N);
  // Alternative way:
  // Name this whatever you want (except for the reserved backend identifiers)
  // clang-format off
  // std::string device_id = "my_custom_device";
  // iqsdk::QRT_ERROR_T status = iqsdk::CustomSimulator::registerCustomInterface<CustomBackend>(device_id, N);
  // assert (status == iqsdk::QRT_ERROR_SUCCESS);
  // iqsdk::DeviceConfig new_device_config(device_id);
  // iqsdk::CustomSimulator generic_simulator(new_device_config);
  // clang-format on
  iqsdk::QRT_ERROR_T status = custom_simulator->ready();
  assert(status == iqsdk::QRT_ERROR_SUCCESS);
  iqsdk::CustomInterface *custom_interface =
      custom_simulator->getCustomBackend();
  assert(custom_interface != nullptr);
  CustomBackend *custom_iqs_instance =
      dynamic_cast<CustomBackend *>(custom_interface);
  assert(custom_iqs_instance != nullptr);
  mbl_q3_1ts();

  std::cout << "Single qubit probabilities" << std::endl;
  for (int i = 0; i < N; ++i) {
    std::cout << "q[" << i
              << "] = " << custom_iqs_instance->psi.GetProbability(i)
              << std::endl;
  }
  delete custom_simulator;
  return 0;
}
