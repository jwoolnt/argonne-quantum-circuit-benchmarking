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
// Implements the Greenberger–Horne–Zeilinger (GHZ) State for an arbitrary
// number of qubits, preparing the state |0...0>+|1...1>
//
//===----------------------------------------------------------------------===//


#include <clang/Quantum/quintrinsics.h>
#include <clang/Quantum/qexpr.h>
#include <quantum_full_state_simulator_backend.h>
#include <qexpr_utils.h>

#include <iostream>
#include <cassert>
#include <vector>


///////////////////////////////////////////////
/// Greenberger–Horne–Zeilinger (GHZ) State ///
///////////////////////////////////////////////

PROTECT QExpr ghz(qlist::QList qs){
    int len = qs.size();
    return (
        // Initialize qs to |0..0>.
        qexpr::map(qexpr::_PrepZ,qs)
        // Prepare the first qubit in state |+>.
        + qexpr::_H(qs[0])
        // Entangle qs[i] with qs[i+1] by mapping CNOT over the QList.
        + qexpr::map(qexpr::_CNOT, qs(0,len-1), qs(1,len)));
        // As an example to understand how this invocation of map works,
        // consider the case where qs has length 3. Then
        //    qs(0,len-1) = { qs[0], qs[1] }
        //    qs(1,len)   = { qs[1], qs[2] }
        // Then this clause will apply _CNOT to each column vector:
        //    _CNOT(qs[0], qs[1]) + _CNOT(qs[1], qs[2])
}

int main() {
    iqsdk::IqsConfig iqs_config;
    iqsdk::FullStateSimulator iqs_device(iqs_config);
    iqsdk::QRT_ERROR_T status = iqs_device.ready();
    assert(status == iqsdk::QRT_ERROR_SUCCESS);


    const int N = 10;
    qbit listable(q, N);

    qexpr::eval_hold(ghz(q));

    // Print out amplitudes
    std::cout << "------- " << N << " qubit GHZ state -------" << std::endl;
    iqsdk::QssIndex zero_vector (std::string(N, '0'));
    iqsdk::QssIndex one_vector  (std::string(N, '1'));

    auto qbit_refs = to_ref_wrappers(q);
    auto amplitude_map = iqs_device.getAmplitudes(qbit_refs,
                                                  {zero_vector, one_vector});
    iqsdk::FullStateSimulator::displayAmplitudes(amplitude_map);

    return 0;
}
