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

/// Quantum Runtime Library APIs
#include <quantum.hpp>

// Clifford Simulator APIs
#include <quantum_clifford_simulator_backend.h>

// Macro flag to enable printing of measurments to screen
//#define DEBUG_MEAS

// This example demonstrate the use of the Clifford simulator for error
// correction. It uses the simplest possible code, the repetition code,
// which maps naturally to a linear nearest-neighbor connectivity. It is also
// the simplest to decode.

// The code works as follows:

// d = data qubit
// a = ancilla qubit
// X = X Pauli operator
// Z = Z Pauli operator
// m = measured to

// qubits:        d -- a -- d -- a -- d-- ...

// stabilizer 0:  Z -> m <- Z
// stabilizer 1:            Z -> m <- Z
// ...
// logical op Z:   Z         Z         Z   ... protected
// logical op X:   X         X         X   ... not protected

// Because the simulation is going to scale our code, the maximum number of
// qubits is declared. The Clifford simulator uses a "sparse" representation of
// the Pauli Tableau, so there is no additional cost for unused qubits. The data
// and ancilla are declared separately for coding clarity.

// Total qubits used is for a given distance is 2 * (distance) - 1.
const unsigned MAX_DISTANCE =
    127; // This will use no more that 255 qubits total.

qbit qdata[MAX_DISTANCE];
qbit anc[MAX_DISTANCE - 1];

// Define the error extraction circuit.
// The distance is used as a template argument throughout.
template <unsigned distance>
quantum_kernel void measureRepCodeSyndrome(cbit result[]) {

  // Prep the ancilla
  for (unsigned i = 0; i < distance - 1; i++) {
    PrepZ(anc[i]);
    H(anc[i]);
  }

  for (unsigned i = 0; i < distance - 1; i++) {
    CZ(qdata[i], anc[i]);
  }

  for (unsigned i = 0; i < distance - 1; i++) {
    CZ(qdata[i + 1], anc[i]);
  }

  // Measure the ancilla
  for (unsigned i = 0; i < distance - 1; i++) {
    H(anc[i]);
    MeasZ(anc[i], result[i]);
  }
}

// This circuit prepare the logical '0' or '1' state based
// on the template argument "init".
template <unsigned distance, bool init> quantum_kernel void prepData() {
  for (int i = 0; i < distance; i++) {
    PrepZ(qdata[i]);
    if (init)
      X(qdata[i]);
  }
}

// To generate noise the qubits are idled for a fixed time.
// This is done by inserting a set of gates which logically form the identity.

// NOTE: The use of O1 will optimize this away. O1 should not be used
// for this example.
template <unsigned T, unsigned distance> quantum_kernel void idle() {
  for (int t = 0; t < T; t++) {
    for (int i = 0; i < distance; i++) {
      X(qdata[i]);
      Y(qdata[i]);
      Z(qdata[i]);
    }
    for (int i = 0; i < distance - 1; i++) {
      X(anc[i]);
      Y(anc[i]);
      Z(anc[i]);
    }
  }
}

// The logical operator is measured as the parity of the measurement of all data
// qubits.
template <unsigned distance> quantum_kernel void measData(cbit result[]) {
  for (int i = 0; i < distance; i++)
    MeasZ(qdata[i], result[i]);
}

// The full quantum process is joined into a single process for efficiency.
template <unsigned distance>
quantum_kernel void runFullRepCodeQuantum(cbit syndrome[], cbit result[]) {
  prepData<distance, false>();
  idle<10, distance>();
  measureRepCodeSyndrome<distance>(syndrome);
  measData<distance>(result);
}

// Next, The decoder is a minority vote on the number of bit flips
// (i.e. majority are not flipped).

template <unsigned distance> bool countMinorityParityDecoding(cbit syndrome[]) {
  int cnt = 0;
  // The syndrome flips the count on/off,
  // i.e. to form "strings" between syndrome bits.
  bool cont_cnt = false;

#ifdef DEBUG_MEAS
  std::cout << "decode:  ";
#endif

  for (int i = 0; i < distance - 1; i++) {
    cont_cnt = cont_cnt ^ syndrome[i];
    cnt += cont_cnt;

#ifdef DEBUG_MEAS
    std::cout << syndrome[i] << " ";
#endif
  }

  // This represents either the majority or minority of bit flipped.
  // Choose the minority by comparing against half the distance.
  if (cnt > distance / 2)
    cnt = distance - cnt;

#ifdef DEBUG_MEAS
  std::cout << " " << cnt % 2 << "\n";
#endif

  return cnt % 2;
}

// This is a small utility for calculating the overall parity.
template <unsigned distance> bool calculateParity(cbit outcomes[]) {
  bool out = false;

#ifdef DEBUG_MEAS
  std::cout << "data:   ";
#endif

  for (int i = 0; i < distance; i++) {
    out = out ^ outcomes[i];

#ifdef DEBUG_MEAS
    std::cout << outcomes[i] << " ";
#endif
  }

#ifdef DEBUG_MEAS
  std::cout << out << "\n";
#endif

  return out;
}

// The code distance is scaled through the use of template recursion.
// There are three template arguments to define:
// 'start' - the code size to start with
// 'end' - the last code size to simulate
// 'inc' - the increment in the code size between calls

// Multiple asyncronous simulations are used to gather statistics.

// This is the base case.
template <int start, int end, int inc>
void runRepCodes(unsigned *counts, unsigned shots,
                 iqsdk::CliffordSimulator sims[], unsigned num_sims,
                 typename std::enable_if<(start > end)>::type * = 0) {}

// This is the recursive call.
template <int start, int end, int inc>
void runRepCodes(unsigned *counts, unsigned shots,
                 iqsdk::CliffordSimulator sims[], unsigned num_sims,
                 typename std::enable_if<(start <= end)>::type * = 0) {

  cbit syndrome[num_sims][start - 1];
  bool results[num_sims][start];
  std::cout << "    Starting on distance " << start << "\n";

  for (int i = 0; i < shots; i++) {
    for (unsigned s = 0; s < num_sims; s++) {
      // Select simulation 's' for the runtime.
      sims[s].ready();
      runFullRepCodeQuantum<start>(syndrome[s], results[s]);
    }
    for (unsigned s = 0; s < num_sims; s++) {
      // Make sure simulation 's' has returned its results.
      sims[s].wait();

      // Note: This use of asyncronous mode is beneficial because
      // the complexity of the Clifford simulation is O(('start')^2),
      // as each gate is O('start') in the worst case,
      // whereas decoding is O('start'). So in this mode, each
      // Clifford simulation is run in parallel, but the main process
      // performs decoding for all of them in sequence. If the complexity
      // of decoding is the same as the Clifford simulation, another
      // parallelization method may be needed to see a speed-up.

      (*counts) += calculateParity<start>(results[s]) ^
                   countMinorityParityDecoding<start>(syndrome[s]);

#ifdef DEBUG_MEAS
      std::cout << "\n";
#endif
    }
  }

  // Call next size of code for recursion.
  runRepCodes<start + inc, end, inc>(counts + 1, shots, sims, num_sims);
}

int main() {

  // Define template parameters for scaling the code distance.
  const unsigned start = 5;
  const unsigned end = 75;
  const unsigned inc = 10;
  const unsigned num_codes = (end - start) / inc + 1;

  // Most gate have afixed gate errors and scale the xyrot error
  // to vary the error rate as this is where error is introduced by the
  // idling method. The error model built into the Clifford Simulator
  // does contain an idle error based on a T1, T2 time, but this is only
  // for necessary idling from ASAP scheduling of the gates.

  double fixed_gate_err = 0.001;

  iqsdk::ErrorRates error_rates;

  error_rates.meas = iqsdk::ErrSpec1Q(fixed_gate_err, 0., 0.);

  error_rates.prep = iqsdk::ErrSpec1Q(fixed_gate_err / 3., fixed_gate_err / 3.,
                                      fixed_gate_err / 3.);

  error_rates.zrot = iqsdk::ErrSpec1Q(fixed_gate_err / 3., fixed_gate_err / 3.,
                                      fixed_gate_err / 3.);

  error_rates.cz = iqsdk::ErrSpec2Q(fixed_gate_err, 0., 0.);

  double err_start = 3e-6;
  double err_inc = 4.;
  const unsigned num_err_rates = 5;

  // Define the number of simultaneous simulations
  // and total number of shots.

  unsigned shots = 1000;
  const unsigned num_sims = 50;
  unsigned shots_per_sim = shots / num_sims;
  double err_rate = err_start;

  // Define the histograms for different code sizes and error rates.
  unsigned histogram[num_err_rates][num_codes] = {0};

  // Run over all error rates, where each rate scales the code size.
  for (int e = 0; e < num_err_rates; e++) {
    std::cout << "Starting on error rate " << err_rate << "\n";

    error_rates.xyrot =
        iqsdk::ErrSpec1Q(err_rate / 3., err_rate / 3., err_rate / 3.);

    // Define the simulators.
    iqsdk::CliffordSimulator cliff_sim[num_sims];
    for (int i = 0; i < num_sims; i++) {
      iqsdk::CliffordSimulatorConfig cliff_config(rand());
      cliff_config.error_rates = error_rates;
      cliff_config.synchronous = false;
      cliff_config.verbose = false;
      cliff_config.use_errors = true;
      cliff_sim[i].initialize(cliff_config);
    }

    // Run the simulation.
    runRepCodes<start, end, inc>(histogram[e], shots_per_sim, cliff_sim,
                                 num_sims);
    err_rate *= err_inc;
  }

  // For this demonstration, the histograms results are simply printed to
  // screen.

  std::cout << "Histogram results for repetition code:\n\n";
  std::cout << "distance/error rate: " << err_start * 30;
  err_rate = err_start;
  for (unsigned e = 1; e < num_err_rates; e++) {
    err_rate *= err_inc;
    std::cout << ", " << err_rate * 30;
  }
  std::cout << "\n";

  unsigned dist = start;
  for (unsigned d = 0; d < num_codes; d++) {
    std::cout << dist << ": " << histogram[0][d];
    for (unsigned e = 1; e < num_err_rates; e++) {
      std::cout << ", " << histogram[e][d];
    }
    std::cout << "\n";
    dist += inc;
  }
}
