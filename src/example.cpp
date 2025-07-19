#include <clang/Quantum/quintrinsics.h>


const int TOTAL_QUBITS = 20;
qbit qubit_register[TOTAL_QUBITS];


quantum_kernel void example_kernel() {
  for (int i = 0; i < TOTAL_QUBITS; i++) {
    PrepZ(qubit_register[i]);
  }

  H(qubit_register[0]);

  for (int i = 0; i < TOTAL_QUBITS - 1; i++) {
    CNOT(qubit_register[i], qubit_register[i + 1]);
  }
}
