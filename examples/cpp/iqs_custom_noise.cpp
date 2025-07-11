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
// This program illustrates the use of the Intel Quantum Simulator (IQS)
// backend with a custom noise model.
//
// The programmer does not need to be familiar with IQS, and no IQS code
// or API needs to be used. The idea is that the user can customize the action
// of every quantum operations by defining appropriate functions.
//
// The action of each operation is divided in three parts:
// - Pre-operation: Here we allow the user to apply one or more of the following
//                  phenomenological noise models:
//                  - dephasing channel
//                  - depolarizing channel
//                  - amplitude damping
//                  - bitflip channel
//                  Each effect is characterized by an intensity parameter.
// - Operation itself: The choice here is whether to apply the ideal operation
//                     or a user-provided process matrix (also known as chi
//                     matrix). In the latter case, the programmer can include
//                     noise effects in the process matrix and thus avoiding
//                     pre- or post-operation actions.
// - Post-operation: Similar to the pre-operation case, the user can apply
//                   one or more of the following phenomenological noise models:
//                  - dephasing channel
//                  - depolarizing channel
//                  - amplitude damping
//                  - bitflip channel
//                  Each effect is characterized by an intensity parameter.
//
// Such specification is provided by means of an object of type iqsdk::IqsCustomOp,
// which can be initialized as follows:
//   IqsCustomOp = {pre_dephasing, pre_depolarizing, pre_amplitude-damping, pre_bitflip          <-- before the operation
//                  process_matrix (std::vector<std::complex<double>> in row-major format),      <-- operation (if "ideal", then empty vector)
//                  label (std::string used a unique tag for the process_matrix),                <-- used to reduce certain overhead
//                  post_dephasing, post_depolarizing, post_amplitude-damping, post_bitflip}     <-- after the operation
// Recall that, for the ideal operation, one can simply use the global object:
//     iqsdk::k_iqs_ideal_op = {0, 0, 0, 0, {}, "ideal", 0, 0, 0, 0}
//
// How does the programmer specify the custom actions?
// They have to write a function for every quantum operation returning the
// appropriate IqsCustomOp object for the given parameters of the quantum
// operation. For example, one may want to use a simplified noise model for the
// one qubit gates by expressing them as ideal gates followed by depolarization.
// At the same time, they may want to use a process matrix describing the
// action of the 2-qubit CZ gates. One may even use different proces matrices
// depending on the qubits involved in the gate.
//
// Below we provide a concrete example to make this approach more concrete.
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <quantum_full_state_simulator_backend.h>
#include <string>

// clang-format off
// The purpose of this is to showcase using a custom backend. Here, we use IQS
// clang-format on
const int N = 3;
qbit q[N];
cbit c[N];

// Specify the custom operations:
// - preparation: ideal
// - RotXY gates: from file if Ry(+90) or Ry(-90), otherwise depolarizing noise (p=0.01) followed by ideal gate
// - CZ gates: from file
// - all other 1- and 2-qubit gates: ideal
// - measurement: ideal
// When the operation is ideal, it is not necessary to add its specification.

// Preparation of one qubit in state |0>.
iqsdk::IqsCustomOp CustomPrep(unsigned q) {return iqsdk::k_iqs_ideal_op;}

// 1-qubit gate representing a rotation around an axis in the XY plane.
// phi determine the axis, gamma the rotation angle.
iqsdk::IqsCustomOp CustomRotXY(unsigned q, double phi, double gamma) {
  bool is_yppi2 = false; // Ry(+pi/2)
  is_yppi2 = is_yppi2 || (std::abs(phi - M_PI/2) < 1e-4 && std::abs(gamma - M_PI/2) < 1e-4);
  bool is_ynpi2 = false; // Ry(-pi/2)
  is_ynpi2 = is_ynpi2 || (std::abs(phi - M_PI/2) < 1e-4 && std::abs(gamma + M_PI/2) < 1e-4);   // phi =  pi/2 , gamma = -pi/2
  is_ynpi2 = is_ynpi2 || (std::abs(phi - 3*M_PI/2) < 1e-4 && std::abs(gamma - M_PI/2) < 1e-4); // phi = -pi/2 , gamma = 3/2 pi
  if (is_yppi2) {
    // Recall that the path is relative to the "chimatrix_directory" in the platform configuration file.
    std::string fname_real = "/qds_yppi2/qpt_real.csv";
    std::string fname_imag = "/qds_yppi2/qpt_imag.csv";
    std::vector<std::complex<double>> chi_matrix = iqsdk::ParseChiMatrixFromCsvFiles(1, fname_real, fname_imag);
    iqsdk::IqsCustomOp op = {0, 0, 0, 0, chi_matrix, "yppi2", 0, 0, 0, 0};
    return op;
  } else if (is_ynpi2) {
    std::string fname_real = "/qds_ynpi2/qpt_real.csv";
    std::string fname_imag = "/qds_ynpi2/qpt_imag.csv";
    std::vector<std::complex<double>> chi_matrix = iqsdk::ParseChiMatrixFromCsvFiles(1, fname_real, fname_imag);
    iqsdk::IqsCustomOp op = {0, 0, 0, 0, chi_matrix, "yppi2", 0, 0, 0, 0};
    return op;
  } else {
    //                 pre-op depolarizing noise
    //                          vvvv
    iqsdk::IqsCustomOp op = {0, 0.01, 0, 0, {}, "", 0, 0, 0, 0};
    return iqsdk::k_iqs_ideal_op;
  }
}

// 2-qubit gate corresponding to a phase applied to q2 controlled by q1 being in |1>.
iqsdk::IqsCustomOp CustomCPhaseRot(unsigned q1, unsigned q2, double gamma) {
  bool is_cphase = false; // CZ
  is_cphase = std::abs(gamma - M_PI) < 1e-4;
  if (is_cphase) {
    std::string fname_real = "/qds_cz/qpt_real.csv";
    std::string fname_imag = "/qds_cz/qpt_imag.csv";
    std::vector<std::complex<double>> chi_matrix = iqsdk::ParseChiMatrixFromCsvFiles(2, fname_real, fname_imag);
    iqsdk::IqsCustomOp op = {0, 0, 0, 0, chi_matrix, "cz", 0, 0, 0, 0};
    return op;
  }
  else
    return iqsdk::k_iqs_ideal_op;
}

//===----------------------------------------------------------------------===//

// Quantum circuit, inspired by a small implementation of an algorithm to
// study Many-Body Localization (MBL).
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

//===----------------------------------------------------------------------===//

int main() {
  // Create an object to specify the IQS configuration.
  // Note the second argument "custom" which specifies that
  // the backend will utilize a user-provided noise model.
  iqsdk::IqsConfig iqs_config(N, "custom");

  // Pass the specification of the custom operations.
  iqs_config.PrepZ = CustomPrep;
  iqs_config.RotationXY = CustomRotXY;
  iqs_config.CPhaseRotation = CustomCPhaseRot;

  iqsdk::FullStateSimulator iqs_device(iqs_config);
  iqsdk::QRT_ERROR_T status = iqs_device.ready();
  assert(status == iqsdk::QRT_ERROR_SUCCESS);
  mbl_q3_1ts();

  std::vector<std::reference_wrapper<qbit>> qids;
  for (int qubit = 0; qubit < N; ++qubit)
    qids.push_back(std::ref(q[qubit]));
  std::vector<double> probs_iqs = iqs_device.getProbabilities(qids);
  std::cout << "Single qubit probabilities" << std::endl;
  for (int i = 0; i < N; ++i) {
    std::cout << "q[" << i << "] = " << probs_iqs[i] << std::endl;
  }

  return 0;
}
