#include <clang/Quantum/quintrinsics.h>
#include <quantum_full_state_simulator_backend.h>

const int total_qubits = 4;
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

iqsdk::IqsCustomOp CustomPrepZ(unsigned q1) {
  return {0, 0, 0, 0, {}, "prep_z", 0, 0, 0, 0};
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
  settings.RotationXY = CustomRotationXY;
  settings.RotationZ = CustomRotationZ;
  settings.ISwapRotation = CustomISwapRotation;
  settings.CPhaseRotation = CustomCPhaseRotation;

  iqsdk::FullStateSimulator quantum_8086(settings);
  if (iqsdk::QRT_ERROR_SUCCESS != quantum_8086.ready())
    return 1;

  // get references to qbits
  std::vector<std::reference_wrapper<qbit>> qids;
  for (int id = 0; id < total_qubits; ++id) {
    qids.push_back(std::ref(qubit_register[id]));
  }

  ghz_total_qubits();

  std::vector<double> probability_map;
  probability_map = quantum_8086.getProbabilities(qids);
  quantum_8086.displayProbabilities(probability_map, qids);
}
