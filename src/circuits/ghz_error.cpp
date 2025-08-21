#include <iostream>
#include <fstream>

#include <clang/Quantum/quintrinsics.h>
#include <quantum_full_state_simulator_backend.h>


const int total_qubits = 5, total_samples = 1000;
qbit qubit_register[total_qubits];
cbit cbit_register[total_qubits];


quantum_kernel void ghz_total_qubits() {
  for (int i = 0; i < total_qubits; i++) {
    PrepZ(qubit_register[i]);
  }

  H(qubit_register[0]);

  for (int i = 0; i < total_qubits - 1; i++) {
    CNOT(qubit_register[i], qubit_register[i + 1]);
  }

  for (int i = 0; i < total_qubits; i++) {
    MeasZ(qubit_register[i], cbit_register[i]);
  }
}


iqsdk::IqsCustomOp CustomPrepZ(unsigned q1) {
  return {0, 0, 0, 0, {}, "prep_z", 0, 0, 0, 0};
}

iqsdk::IqsCustomOp CustomMeasZ(unsigned q1) {
  if (q1 < 2) {
    return {0, 0.01, 0, 0.01, {}, "meas_z", 0, 0, 0, 0};
  } else {
    return iqsdk::k_iqs_ideal_op;
  }
}

iqsdk::IqsCustomOp CustomRotationXY(unsigned qubit, double phi, double gamma) {
  return {0, 0, 0, 0, {}, "rotation_x_y", 0, 0, 0, 0};
}

iqsdk::IqsCustomOp CustomRotationZ(unsigned qubit, double gamma) {
  return {0, 0, 0, 0, {}, "rotation_z", 0, 0, 0, 0};
}

iqsdk::IqsCustomOp CustomISwapRotation(unsigned qubit_1, unsigned qubit_2, double gamma) {
  return {0, 0, 0, 0, {}, "i_swap_rotation", 0, 0, 0, 0};
}

iqsdk::IqsCustomOp CustomCPhaseRotation(unsigned qubit_1, unsigned qubit_2, double gamma) {
  return {0, 0, 0, 0, {}, "c_phase_rotation", 0, 0, 0, 0};
}


int main() {
  iqsdk::IqsConfig settings(total_qubits, "custom");
  settings.PrepZ = CustomPrepZ;
  settings.MeasZ = CustomMeasZ;
  settings.RotationXY = CustomRotationXY;
  settings.RotationZ = CustomRotationZ;
  settings.ISwapRotation = CustomISwapRotation;
  settings.CPhaseRotation = CustomCPhaseRotation;

  iqsdk::FullStateSimulator quantum_8086(settings);

  std::ofstream file("results/ghz_error/correlation.csv");
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file" << std::endl;
    return 1;
  }

  file << "qubit_0";
  for (int i = 1; i < total_qubits; i++) {
    file << ',' << "qubit_" << i;
  }
  file << '\n';

  for (int sample = 0; sample < total_samples; sample++) {
    if (iqsdk::QRT_ERROR_SUCCESS != quantum_8086.ready())
      return 1;

    ghz_total_qubits();

    file << cbit_register[0];
    for (int i = 1; i < total_qubits; i++) {
      file << ',' << cbit_register[i];
    }
    file << '\n';
  }

  file.close();
}
