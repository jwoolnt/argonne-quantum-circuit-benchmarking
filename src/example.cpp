#include <clang/Quantum/quintrinsics.h>


const int total_qubits = 5;
qbit qubit_register[total_qubits];


quantum_kernel void example_kernel() {
  for (int i = 0; i < total_qubits; i++) {
    PrepZ(qubit_register[i]);
  }

  H(qubit_register[0]);

  for (int i = 0; i < total_qubits - 1; i++) {
    CNOT(qubit_register[i], qubit_register[i + 1]);
  }
}
