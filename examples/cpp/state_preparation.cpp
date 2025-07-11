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
// This file shows how to prepare a multi-quit state that is specified by a
// compile-time string (a DataList).
//
// The top-level function is prepState(state,qs), which takes as input (1) a
// state specified as a string/DatatList, of the form "|c...c>"; and (2) a
// QList. Each character 'c' specifies a basis state (up to a global phase):
//
//     specification       basis       state
//     --------------------------------------
//     '0'                 Z           |0>
//     '1'                 Z           |1>
//     '+'                 X           1/sqrt(2)(|0> + |1>)
//     '-'                 X           1/sqrt(2)(|0> - |1>)
//     'R'                 Y           1/sqrt(2)(|0> + i|1>)
//     'L'                 Y           1/sqrt(2)(|0> - i|1>)
//
//===----------------------------------------------------------------------===//


#include <clang/Quantum/quintrinsics.h>
#include <clang/Quantum/qexpr.h>
#include <clang/Quantum/qlist.h>
#include <clang/Quantum/datalist.h>

#include <qexpr_utils.h>

#include <vector>
#include <iostream>
#include <cassert>

#include <quantum_full_state_simulator_backend.h>



/// @brief      Prepares multiple qubits in the states specified by the DataList
///             src
/// @param qs   A QList
/// @param src  A DataList of the form "|c...c>" where each character c is in the
///             set {'0','1','+','-','R','L'}.
///             Requires src.size() == qs.size() + 2.
QExpr prepState(const datalist::DataList src, const qlist::QList qs);

////////////////////////
/// Helper functions ///
////////////////////////

/// @brief      Returns a QExpr that prepares the qubit q in the state specified by c.
/// @param q    A qubit
/// @param c    A character in the set {'0','1','+','-','L','R'}
QExpr stateToQExpr(qbit& q, const char c) {
    return
        qexpr::cIf(c == '0', qexpr::_PrepZ(q),
        qexpr::cIf(c == '1', qexpr::_PrepZ(q) + qexpr::_X(q),
        qexpr::cIf(c == '+', qexpr::_PrepX(q),
        qexpr::cIf(c == '-', qexpr::_PrepX(q) + qexpr::_Z(q),
        qexpr::cIf(c == 'R', qexpr::_PrepY(q),
        qexpr::cIf(c == 'L', qexpr::_PrepY(q) + qexpr::_Z(q),
        qexpr::exitAtCompile("prepState: Expected a character in the set {'0','1','+','-','R','L'}.")
        ))))));
}

/// @brief      Prepares multiple qubits in the states specified by the DataList src
/// @param qs   A QList
/// @param src  A DataList with each character in the set {'0','1','+','-','R','L'}.
///             Requires src.size() == qs.size()
/// @return     A QExpr that prepares each qubit qs[i] in state src[i]
QExpr multiStateToQExpr(const qlist::QList qs, const datalist::DataList src) {
    return qexpr::cIf(qs.size() == 0,
                        qexpr::identity(),
                        stateToQExpr(qs[0], src[0])
                            + multiStateToQExpr(qs>>1, src>>1)
    );
}


QExpr prepState(const datalist::DataList src, const qlist::QList qs) {
    return
        qexpr::qassert(src[0] == '|',
                "prepState: Expected a datalist of the form |state>")
        +
        qexpr::qassert(src[src.size()-1] == '>',
                "prepState: Expected a datalist of the form |state>")
        +
        qexpr::qassert(src.size() == qs.size() + 2,
                datalist::DataList("prepState: Expected a state of size ")
                + datalist::DataList(qs.size()))
        +
        // Strip the ket from the datalist
        multiStateToQExpr(qs, src("|",">") >> 1)
        ;
}

int main() {

    iqsdk::IqsConfig iqs_config;
    iqsdk::FullStateSimulator iqs_device(iqs_config);
    iqsdk::QRT_ERROR_T status = iqs_device.ready();
    assert(status == iqsdk::QRT_ERROR_SUCCESS);

    const int N = 5;
    qbit listable(q,N);
    qexpr::eval_hold(prepState("|0+1-1>",q));

    auto refs = to_ref_wrappers(q);
    auto probs = iqs_device.getProbabilities(refs, {}, 0.01);
    iqsdk::FullStateSimulator::displayProbabilities(probs);

    return 0;
}
