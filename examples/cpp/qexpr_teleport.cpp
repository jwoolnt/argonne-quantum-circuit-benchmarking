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
// This tutorial introduces the main concepts of FLEQ using quantum
// teleportation. Quantum teleportation allows one actor, Alice, to send quantum
// information to another actor, Bob, in the form of a quantum state. The protocol
// proceeds as follows:

// 1. Alice and Bob each start with one half of a Bell pair in the state
//    1/sqrt(2)(|00>+|11>).
// 2. Alice prepares her state |phi>= a|0> + b|1>,
//    resulting in the three-qubit system 1/sqrt(2)|phi>⊗(|00>+|11>).
// 3. Alice entangles her state with her half of the Bell pair and measures both
//    qubits, producing bits x and y, and leaving Bob's half of
//    the Bell pair in one of the following four states:
//
//      x   y   Bob's state
//    -----------------
//      0   0   a|0> + b|1>
//      0   1   a|0> - b|1>
//      1   0   a|1> + b|0>
//      1   1   a|1> - b|0>
//
// 4. Finally, Alice sends the classical bits x and y to Bob, who
//    uses that information to correct his state to Alice's original
//    |phi> = a|0> + b|1>.
//
//===----------------------------------------------------------------------===//

#include <clang/Quantum/quintrinsics.h>
#include <quantum_full_state_simulator_backend.h>
#include <clang/Quantum/qexpr.h>
#include <qexpr_utils.h>

#include <iostream>
#include <cassert>
#include <vector>
#include <random>


///////////////////////
/// Building Blocks ///
///////////////////////

// Prepare a Bell00 state |00> + |11>.
QExpr bell00(qbit& a, qbit& b) {
    return qexpr::_PrepZ(a)
         + qexpr::_PrepZ(b)
         + qexpr::_H(a)
         + qexpr::_CNOT(a,b);
}

// Entangle q and a and measure both, writing the results to x and y
// respectively.
QExpr alice(qbit& q, qbit& a, bool& x, bool& y) {
    return qexpr::_CNOT(q, a)
         + qexpr::_H(q)
         + qexpr::_MeasZ(q, x)
         + qexpr::_MeasZ(a, y);
}

// Use x and y to apply corrections to the qubit b.
PROTECT QExpr bob(qbit& b, bool &x, bool &y) {
    return qexpr::cIf(y, qexpr::_X(b), qexpr::identity())
         + qexpr::cIf(x, qexpr::_Z(b), qexpr::identity());
}


double randomDoubleBetweenZeroAndTwoPi() {
    std::random_device rd;  // Used to seed the random number generator
    std::mt19937 gen(rd()); // Mersenne Twister PRNG
    std::uniform_real_distribution<double> dis(0.0, 2.0 * M_PI);
    return dis(gen);
}
// Prepare a state |phi> by performing an X rotation around a random angle
PROTECT QExpr prepPhi(qbit& q) {
    double theta = randomDoubleBetweenZeroAndTwoPi();
    std::cout << "Using angle " << theta << "\n";
    return qexpr::_PrepZ(q) + qexpr::_RX(q, theta);
}


// Prepare a GHZ state
PROTECT QExpr ghz(qlist::QList qs){
    int len = qs.size();
    return (qexpr::map(qexpr::_PrepZ, qs)
          + qexpr::_H(qs[0])
          + qexpr::map(qexpr::_CNOT, qs(0,len-1), qs(1,len)));
}

//////////////////////////////////////////
/// Single-qubit quantum teleportation ///
//////////////////////////////////////////

// Perform a hybrid classical-quantum teleportation algorithm
// over 1 qubit.
void teleport1(iqsdk::FullStateSimulator& device) {

    qbit q;
    qbit a;
    qbit b;

    // Prepare qubits a and b in a bell state
    qexpr::eval_hold(bell00(a,b));

    // Alice prepares her qubit q in the state |phi>
    qexpr::eval_hold(prepPhi(q));

    // Record the state Alice prepared
    auto q_ref = to_ref_wrappers(qlist::QList(q));
    auto probabilitiesBefore = device.getProbabilities(q_ref);

    // Alice entangles her state q with a, and sends measurement
    // results x and y to Bob
    bool x;
    bool y;
    qexpr::eval_hold(alice(q, a, x, y));

    // Bob uses x and y to correct his qubit b
    qexpr::eval_hold(bob(b, x, y));

    // At the end, b should be in the state |phi>, up to a global phase
    auto b_ref = to_ref_wrappers(qlist::QList(b));
    auto probabilitiesAfter = device.getProbabilities(b_ref);

    std::cout << "Before teleportation, qubit q has distribution:\n";
    iqsdk::FullStateSimulator::displayProbabilities(probabilitiesBefore, q_ref);
    std::cout << "After teleportation, qubit b has distribution:\n";
    iqsdk::FullStateSimulator::displayProbabilities(probabilitiesAfter, b_ref);
}

//////////////////////////////////////////////////
/// Teleportation in a single QExpr using bind ///
//////////////////////////////////////////////////


// The function teleport1() above contains multiple evaluation calls to
// eval_hold(); it itself is a classical function that interacts with the
// quantum runtime. If a user does not need to report the output of the first
// state, can they implement teleportation as a single QExpr function?

// An initial **incorrect** attempt in doing this is to write a QExpr function that simply
// joins the three modular components of the teleportation protocol.

// Incorrect: Attempt to implement teleportation in a single QBB with join
PROTECT QExpr teleport1_join(qbit& q, qbit& a, qbit& b) {
    bool x = false;
    bool y = false;
    return bell00(a,b)
            + alice(q, a, x, y)
            + bob(b,x,y);
}

// When we try to run this algorithm, half the time we will observe b in the
// state |0> and half the time we will observe it in the state |1>. This is an
// indication that the corrections in Bob's part of the protocol are not being
// applied correctly. Indeed, when we print out the values of x and y before Bob
// performs his corrections, we see that their values are *always* 0.

// The reason for this is that the measurement in alice() is occurring
// within the same Quantum Basic Block (QBB) as the conditional in bob().
// However, measurement results are not written to classical variables x and
// y until the end of a QBB. Thus, the measurement results do not propagate
// to x and y before Bob tries to use them.

// Incorrect: Attempt to use teleport1_join() to implement teleportation
void teleport1_bad(iqsdk::FullStateSimulator& device) {
    qbit q;
    qbit a;
    qbit b;

    qexpr::eval_hold(qexpr::_PrepZ(q) + qexpr::_X(q) + teleport1_join(q,a,b));

    // At the end, b should be in the state |phi>, up to a global phase
    auto b_ref = to_ref_wrappers(qlist::QList(b));
    auto probabilitiesAfter = device.getProbabilities(b_ref);

    std::cout << "Expecting state |1>\n";
    std::cout << "After teleportation, Bob obtains state:\n";
    iqsdk::FullStateSimulator::displayProbabilities(probabilitiesAfter, b_ref);
}

// The solution is to insert a barrier between Alice's protocol and Bob's protocol
// to ensure they happen within separate QBBs. This can be achieved via the
// bind function, an analog of the usual join function that combines two
// quantum kernel expressions. Whereas join takes two quantum kernel expressions
// and combines them sequentially in the same quantum basic block, bind
// produces separate QBBs, executing one after the other. Analogous to the notation
// e1 + e2 for joining quantum kernel expressions in sequence, users can write
// e1 << e2 for binding quantum kernel expressions in sequence.

// Correct: add a barrier between Alice's measurements and Bob's corrections
PROTECT QExpr teleport1_bind(qbit& q, qbit& a, qbit& b) {
    bool x = false;
    bool y = false;
    return (bell00(a,b) + alice(q, a, x, y))
            << bob(b,x,y);
}

// Single-qubit teleportation using teleport1_bind()
void teleport1_good(iqsdk::FullStateSimulator& device) {
    qbit q;
    qbit a;
    qbit b;

    qexpr::eval_hold((qexpr::_PrepZ(q) + qexpr::_X(q)) // prepare |phi>
                    + teleport1_bind(q,a,b));

    // At the end, b should be in the state |phi>, up to a global phase
    auto b_ref = to_ref_wrappers(qlist::QList(b));
    auto probabilitiesAfter = device.getProbabilities(b_ref);

    std::cout << "Expecting state |1>\n";
    std::cout << "After teleportation, Bob obtains state:\n";
    iqsdk::FullStateSimulator::displayProbabilities(probabilitiesAfter, b_ref);
}

/////////////////////////////
/// N-qubit Teleportation ///
/////////////////////////////

// Extending single-qubit quantum teleportation to a protocol that teleports N
// qubits for an arbitrary N is not difficult. The protocol requires N pairs of
// qubits in a Bell state and performs the single-qubit teleportation sequence
// for each qubit.

// Implement N-qubit teleportation by sequentially teleporting each of the N
// qubits in the state.
// Introduces N barriers, once in each occurrence of teleport1_bind.
// Assume qs.size() == as.size() == bs.size()
QExpr teleport_sequential(qlist::QList qs, qlist::QList as, qlist::QList bs) {
    return qexpr::cIf(qs.size() == 0,
                        // if qs is empty:
                            qexpr::identity(),
                        // if qs is non-empty:
                            teleport1_bind(qs[0], as[0], bs[0])
                            << teleport_sequential(qs+1, as+1, bs+1)
    );
}

// Perform a hybrid classical-quantum teleportation algorithm
// over N qubits using teleport_sequential
void teleportN_sequential(iqsdk::FullStateSimulator& device) {
    const int N = 2;
    qbit listable(qs, N);
    qbit listable(as, N);
    qbit listable(bs, N);

    qexpr::eval_hold(ghz(qs) // Prepare |phi>
                    + teleport_sequential(qs, as, bs));

    // At the end, bs should be in the state |phi>, up to a global phase
    auto outputRefs = to_ref_wrappers(bs);
    auto probsAfter = device.getProbabilities(outputRefs, {}, 0.01);

    std::cout << "Expecting GHZ state |0...0> + |1...1>\n";
    std::cout << "Qubits bs after teleportation:\n";
    iqsdk::FullStateSimulator::displayProbabilities(probsAfter);

}

// Because each call to teleport1_bind has a barrier in the form of a bind,
// teleport_parallel(qs, as, bs) will result in N quantum basic blocks (QBBs)
// where N = qs.size(). While such barriers are logically valid, they prevent
// the compiler from optimizing across boundaries, which can make compilation
// extremely slow. It can also result in less-ideal placements (which are
// determined for each QBB individually) and scheduling. Logically, the N-qubit
// teleportation protocol really should have three separate components:

// 1. First, Alice and Bob prepare their joint Bell states.
// 2. Second, Alice prepares her state |phi> and measures her qubits.
// 3. Finally, Bob receives Alice's measurements and performs his own corrections.

// These three states could be achieved using their own recursive functions over
// the qubit lists, as in teleport_sequential(). But each of these three cases
// has a similar structure. Consider:

// 1. Preparing the joint Bell states takes as input two QList values as
//    and bs and maps bell00() over each pair as[i] and bs[i].

// 2. Alice's preparations involve first preparing the state |phi> and then
//    mapping the function
//      alice(qbit& q, qbit& a, bool& x, bool& y)
//    over qs[i], as[i]], and two boolean arrays xs[i] and ys[i].

// 3. Bob's corrections involve mapping the function
//      bob(qbit& b, bool x, bool y)
//    over bs, xs, and ys.

// Each of these three cases (as well as teleport_sequential() itself) involves
// mapping a single-qubit QExpr function over one or more QList values or
// arrays. In fact, this is such a commonly occurring pattern that FLEQ provides
// a higher-order functional utility called qexpr::map() in the header file
// qexpr-utils.h.

// Implement N-qubit teleportation by breaking the algorithm into three steps,
// each of which is distributed across all N qubits.
// Assume qs.size() == as.size() == bs.size()
PROTECT
QExpr teleport_parallel(QExpr phi, qlist::QList qs, qlist::QList as, qlist::QList bs) {
    bool xs[qs.size()];
    bool ys[qs.size()];

    return qexpr::map(bell00, as, bs)
        << (phi + qexpr::map(alice, qs, as, xs, ys))
        << qexpr::map(bob, bs, xs, ys);
}

// Perform a hybrid classical-quantum teleportation algorithm
// over N qubits using teleport_parallel
void teleportN(iqsdk::FullStateSimulator& device) {

    const int N = 3;
    qbit listable(qs, N);
    qbit listable(as, N);
    qbit listable(bs, N);

    // Teleportation with |phi>=1/sqrt(2)(|0...0> + |1...1>)
    qexpr::eval_hold(teleport_parallel(ghz(qs), qs, as, bs));

    // At the end, bs should be in the state |phi>=1/sqrt(2)(|0...0> + |1...1>)
    // (up to a global phase)
    auto outputRefs = to_ref_wrappers(bs);
    auto probsAfter = device.getProbabilities(outputRefs, {}, 0.01);

    std::cout << "Expecting GHZ state |0...0> + |1...1>\n";
    std::cout << "Qubits bs after teleportation:\n";
    iqsdk::FullStateSimulator::displayProbabilities(probsAfter);
}

int main() {
    iqsdk::IqsConfig iqs_config;
    iqsdk::FullStateSimulator iqs_device(iqs_config);
    iqsdk::QRT_ERROR_T status = iqs_device.ready();
    assert(status == iqsdk::QRT_ERROR_SUCCESS);

    // Uncomment each line individually to try each approach

    //teleport1(iqs_device);
    //teleport1_bad(iqs_device);
    //teleport1_good(iqs_device);
    //teleportN_sequential(iqs_device);
    teleportN(iqs_device);

    return 0;
}
