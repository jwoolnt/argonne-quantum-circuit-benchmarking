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
// Here we show how to personalize the simulator backend in two ways.
// 1) by using the API for custom-noise IQS backend
// 2) by writing a custom backend with the IQSDK API so that its behavior is as for 1).
//
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>

#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <cmath>
#include <string.h>

// For the custom backend API, also including the IQS header.
#include <quantum_custom_backend.h>
#include <qureg.hpp>

// For the custom-noise IQS backend.
#include <quantum_full_state_simulator_backend.h>

//===----------------------------------------------------------------------===//
// clang-format off
// The purpose of this is to showcase using a custom backend. Here, we use IQS
// clang-format on

const double k_depol_rate = 0.02;
const int default_num_ensemble_states = 10000;
const std::size_t k_rng_seed = 12347;

const int N = 5;
qbit q[N];
cbit c[N];

quantum_kernel void circuit() {
  for (int i = 0; i < N; i++) {
    PrepZ(q[i]);
    H(q[i]);
  }

  X(q[0]);
  Y(q[2]);
  CNOT(q[0], q[1]);
  RZ(q[0], 9.563581772879);
  RZ(q[1], 8.0);
  RY(q[2], 2.8);
  RY(q[3], -0.8);
  RX(q[4], 0.4);
  H(q[0]);
  CNOT(q[0], q[1]);
  CNOT(q[4], q[2]);
  CNOT(q[3], q[1]);

  std::vector<double> angles = {-0.2, 0.4, 0.7, -1.1, 0.3};
  for (int i = 0; i < N; i++) {
    RY(q[i], angles[i]);
  }
  CNOT(q[1], q[3]);
  CNOT(q[4], q[2]);
  CNOT(q[3], q[1]);
  CNOT(q[0], q[2]);
  CNOT(q[4], q[0]);

  angles = {-0.3, -0.5, 1.7, 2.1, -0.9};
  for (int i = 0; i < N; i++) {
    RX(q[i], angles[i]);
  }
  CNOT(q[1], q[3]);
  CNOT(q[3], q[2]);
  CNOT(q[4], q[1]);
  CNOT(q[1], q[0]);
  CNOT(q[4], q[0]);

  RX(q[0], 1.94);
  RY(q[1], 1.04);
  RZ(q[2], 2.56);
  RY(q[3], 2.56);
  RX(q[4], 2.56);
  H(q[1]);
}

//===----------------------------------------------------------------------===//
// -- Read the chi matrices once.
// In general one would need to write a function to parse the process matrix.
// For convenience, we use the utility function defined in "quantum_full_state_simulator_backend.h".
// Recall that the path is relative to the "chimatrix_directory" in the platform configuration file.
const std::vector<std::complex<double>> _chi_vector_yppi2 =
  iqsdk::ParseChiMatrixFromCsvFiles(1, "/qds_yppi2/qpt_real.csv", "/qds_yppi2/qpt_imag.csv");
const std::vector<std::complex<double>> _chi_vector_ynpi2 =
  iqsdk::ParseChiMatrixFromCsvFiles(1, "/qds_ynpi2/qpt_real.csv", "/qds_ynpi2/qpt_imag.csv");
const std::vector<std::complex<double>> _chi_vector_cz =
  iqsdk::ParseChiMatrixFromCsvFiles(2, "/qds_cz/qpt_real.csv", "/qds_cz/qpt_imag.csv");

//===----------------------------------------------------------------------===//
// Here we use the API for custom-noise IQS backend.
// Further down, we will use the general custom backend API to implement the same behavior.

// Specify the custom operations:
// - preparation: ideal
// - RotXY gates: from file if Ry(+90) or Ry(-90), otherwise depolarizing noise (p=0.01) followed by ideal gate
// - CZ gates: from file, otherwise 2q gates are ideal
// - all other 1- and 2-qubit gates: ideal
// - measurement: ideal
// When the operation is ideal, it is not necessary to add its specification.

// Preparation of one qubit in state |0>.
iqsdk::IqsCustomOp CustomPrep(unsigned q) {return iqsdk::k_iqs_ideal_op;}

// 1- qubit gate representing a rotation around the Z axis.
// gamma is the rotation angle.
iqsdk::IqsCustomOp CustomRotZ(unsigned q, double gamma) {return iqsdk::k_iqs_ideal_op;}

// 1-qubit gate representing a rotation around an axis in the XY plane.
// phi determine the axis, gamma the rotation angle.
iqsdk::IqsCustomOp CustomRotXY(unsigned q, double phi, double gamma) {
  bool is_yppi2 = false; // Ry(+pi/2)
  is_yppi2 = is_yppi2 || (std::abs(phi - M_PI/2) < 1e-4 && std::abs(gamma - M_PI/2) < 1e-4);
  bool is_ynpi2 = false; // Ry(-pi/2)
  is_ynpi2 = is_ynpi2 || (std::abs(phi - M_PI/2) < 1e-4 && std::abs(gamma + M_PI/2) < 1e-4);   // phi =  pi/2 , gamma = -pi/2
  is_ynpi2 = is_ynpi2 || (std::abs(phi - 3*M_PI/2) < 1e-4 && std::abs(gamma - M_PI/2) < 1e-4); // phi = -pi/2 , gamma = 3/2 pi
  if (is_yppi2) {
    iqsdk::IqsCustomOp op = {0, 0, 0, 0, _chi_vector_yppi2, "yppi2", 0, 0, 0, 0};
    return op;
  } else if (is_ynpi2) {
    iqsdk::IqsCustomOp op = {0, 0, 0, 0, _chi_vector_ynpi2, "ynpi2", 0, 0, 0, 0};
    return op;
  } else {
    //                    pre-op depolarizing noise
    //                          vvvvvvvvvvvv
    iqsdk::IqsCustomOp op = {0, k_depol_rate, 0, 0, {}, "", 0, 0, 0, 0};
    return op;
  }
}

// 2-qubit gate corresponding to a phase applied to q2 controlled by q1 being in |1>.
iqsdk::IqsCustomOp CustomCPhaseRot(unsigned q1, unsigned q2, double gamma) {
  bool is_cphase = false; // CZ
  is_cphase = std::abs(gamma - M_PI) < 1e-4;
  if (is_cphase) {
    iqsdk::IqsCustomOp op = {0, 0, 0, 0, _chi_vector_cz, "cz", 0, 0, 0, 0};
    return op;
  }
  else
    return iqsdk::k_iqs_ideal_op;
}

//===----------------------------------------------------------------------===//
// Here we use the API for custom backends to reproduce the same behavior as above.

class CustomBackend : public iqsdk::CustomInterface {
public:
  iqs::QubitRegister<ComplexDP> psi;
  iqs::RandomNumberGenerator<double> rng;
  iqs::CM4x4<ComplexDP> chi_yppi2, chi_ynpi2, chi_depol;
  iqs::CM16x16<ComplexDP> chi_cz;

  CustomBackend(int num_qubits)
      : psi(iqs::QubitRegister<ComplexDP>(num_qubits, "base", 0)) {
    rng.SetSeedStreamPtrs(k_rng_seed);
    psi.SetRngPtr(&rng);

    for (unsigned i_row=0; i_row<4; i_row++)
    for (unsigned i_col=0; i_col<4; i_col++)
    {
        this->chi_yppi2(i_row, i_col) = _chi_vector_yppi2[i_row*4 + i_col];
        this->chi_ynpi2(i_row, i_col) = _chi_vector_ynpi2[i_row*4 + i_col];
    }
    for (unsigned i_row=0; i_row<16; i_row++)
    for (unsigned i_col=0; i_col<16; i_col++)
    {
        this->chi_cz(i_row, i_col) = _chi_vector_cz[i_row*16 + i_col];
    }
    this->chi_yppi2.SolveEigenSystem();
    this->chi_ynpi2.SolveEigenSystem();
    this->chi_cz.SolveEigenSystem();
    //
    chi_depol = iqs::Get1QubitDepolarizingChiMatrix<ComplexDP>(k_depol_rate);
    chi_depol.SolveEigenSystem();
  }

  void PrepZ(qbit q) {
    // Preparation via measurement and, possibly, bit flip.
    double prob = psi.GetProbability(q);
    double r = 1;
    if (psi.GetRngPtr() != nullptr)
    {
      psi.GetRngPtr()->UniformRandomNumbers(&r, 1, 0, 1, "state");
    }
    else
    {
      r = (double) std::rand()/RAND_MAX;
    }
    //
    if (r>prob) {
      psi.CollapseQubit(q, false); // --> |0>
      psi.Normalize();
    } else {
      psi.CollapseQubit(q, true); // --> |1>
      psi.Normalize();
      psi.ApplyPauliX(q);
    }
  }

  // Ideal RotZ.
  void RZ(qbit q, double angle) {
    psi.ApplyRotationZ(q, angle);
  }

  // Depolarizing noise (p=0.01) followed by ideal RotXY.
  // Exception: if Ry(+90) or Ry(-90) take the process matrix specification from CSV files.
  void RXY(qbit q, double phi, double theta) {
    bool is_yppi2 = false; // Ry(+pi/2)
    is_yppi2 = is_yppi2 || (std::abs(phi - M_PI/2) < 1e-4 && std::abs(theta - M_PI/2) < 1e-4);
    bool is_ynpi2 = false; // Ry(-pi/2)
    is_ynpi2 = is_ynpi2 || (std::abs(phi - M_PI/2) < 1e-4 && std::abs(theta + M_PI/2) < 1e-4);   // phi =  pi/2 , gamma = -pi/2
    is_ynpi2 = is_ynpi2 || (std::abs(phi - 3*M_PI/2) < 1e-4 && std::abs(theta - M_PI/2) < 1e-4); // phi = -pi/2 , gamma = 3/2 pi
    if (is_yppi2)
        psi.ApplyChannel(q, chi_yppi2);
    else if (is_ynpi2)
        psi.ApplyChannel(q, chi_ynpi2);
    else {
        psi.ApplyChannel(q, chi_depol);
        psi.ApplyRotationXY(q, phi, theta);
    }
  }

  void CPhase(qbit ctrl, qbit target, double angle) {
    bool is_cphase = false; // CZ
    is_cphase = std::abs(angle - M_PI) < 1e-4;
    if (is_cphase)
      psi.ApplyChannel(ctrl, target, chi_cz);
    else
      psi.ApplyCPhaseRotation(ctrl, target, -angle);
  }

  void SwapA(qbit q1, qbit q2, double angle) {
    iqs::TinyMatrix<ComplexDP, 2, 2, 32> gate_matrix;
    gate_matrix(0, 0) = gate_matrix(1, 1) = {
        0.5 * (1.0 + std::cos(angle)), 0.5 * std::sin(angle)};
    gate_matrix(0, 1) = gate_matrix(1, 0) = {
        0.5 * (1.0 - std::cos(angle)), -0.5 * std::sin(angle)};
    psi.ApplyISwapRotation(q1, q2, gate_matrix);
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

//===----------------------------------------------------------------------===//

int main(int argc, char *argv[]) {

  // Set name of output file.
  std::string filename = "TEMP_out.txt";
  if (argc > 1)
    filename = argv[1];
  // Set the number of states in the ensemble;
  int num_ensemble_states = default_num_ensemble_states;
  if (argc > 2)
    num_ensemble_states = std::atoi(argv[2]);

  std::ofstream myfile;
  myfile.open (filename);
  myfile << "ensemble_size\tq\tcustom\tiqs\n";

  // Custom backend API.
  iqsdk::CustomSimulator *custom_simulator =
      iqsdk::CustomSimulator::createSimulator<CustomBackend>("my_custom_device", N);
  iqsdk::QRT_ERROR_T status = custom_simulator->ready();
  assert(status == iqsdk::QRT_ERROR_SUCCESS);
  iqsdk::CustomInterface *custom_interface =
      custom_simulator->getCustomBackend();
  assert(custom_interface != nullptr);
  CustomBackend *custom_iqs_instance =
      dynamic_cast<CustomBackend *>(custom_interface);
  assert(custom_iqs_instance != nullptr);

  std::vector<double> single_state_probs(N);
  std::vector<std::reference_wrapper<qbit>> qids;
  for (int qubit = 0; qubit < N; ++qubit)
    qids.push_back(std::ref(q[qubit]));

  std::vector<double> probs_custom(N*num_ensemble_states, 0);
  for (int k=0; k<num_ensemble_states; k++) {
    circuit();
    for (int q=0; q<N; q++) {
      probs_custom[k*N+q] = custom_iqs_instance->psi.GetProbability(q);
    }
  }

  // Custom-noise IQS API.
  iqsdk::IqsConfig iqs_config(N, "custom", false, k_rng_seed); // false is for the "verbose" option
  iqs_config.PrepZ = CustomPrep;
  iqs_config.RotationZ = CustomRotZ;
  iqs_config.RotationXY = CustomRotXY;
  iqs_config.CPhaseRotation = CustomCPhaseRot;
  iqsdk::FullStateSimulator iqs_device(iqs_config);
  status = iqs_device.ready();
  assert(status == iqsdk::QRT_ERROR_SUCCESS);

  std::vector<double> probs_iqs(N*num_ensemble_states, 0);
  for (int k=0; k<num_ensemble_states; k++) {
    circuit();
    //single_state_probs = iqs_device.getProbabilities(qids);
    single_state_probs = iqs_device.getSingleQubitProbs(qids);
    for (int q=0; q<N; q++) {
      probs_iqs[k*N+q] = single_state_probs[q];
    }
  }

  // Compute the incoherent average of the single-qubit probabilities.
  std::vector<double> sum_probs_custom(N, 0);
  std::vector<double> sum_probs_iqs(N, 0);
  for (int k=0; k<num_ensemble_states; k++) {
    for (int q=0; q<N; q++) {
      sum_probs_custom[q] += probs_custom[k*N+q];
      sum_probs_iqs[q] += probs_iqs[k*N+q];
      myfile << k+1 << "\t" << q << "\t"
             << sum_probs_custom[q]/(double)(k+1) << "\t"
             << sum_probs_iqs[q]/(double)(k+1) << "\n";
    }
  }
  myfile.close();

  std::cout << "\nSingle qubit probabilities (averaged over "
            << "an ensemble of " << num_ensemble_states
            << " states):\n"
            << "   (custom backend , custom-noise IQS )\n";
  for (int q = 0; q < N; ++q) {
    std::cout << "q[" << q
              << "] = ( " << sum_probs_custom[q]/(double)num_ensemble_states
              << " , " << sum_probs_iqs[q]/(double)num_ensemble_states
              << " )\n";
  }
  delete custom_simulator;

  return 0;
}
