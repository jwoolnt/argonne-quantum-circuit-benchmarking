#include <clang/Quantum/quintrinsics.h>


const int TOTAL_QUBITS = 20;
qbit qubit_register[TOTAL_QUBITS];


#define GHZ(N) \
quantum_kernel void ghz_##N() { \
  for (int i = 0; i < N; i++) { \
    PrepZ(qubit_register[i]); \
  } \
  H(qubit_register[0]); \
  for (int i = 0; i < N - 1; i++) { \
    CNOT(qubit_register[i], qubit_register[i + 1]); \
  } \
}

GHZ(1)
GHZ(2)
GHZ(3)
GHZ(4)
GHZ(5)
GHZ(6)
GHZ(7)
GHZ(8)
GHZ(9)
GHZ(10)
GHZ(11)
GHZ(12)
GHZ(13)
GHZ(14)
GHZ(15)
GHZ(16)
GHZ(17)
GHZ(18)
GHZ(19)
GHZ(20)

#undef GHZ
