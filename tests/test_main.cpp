#include "FunctionSpeaker.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// Helper test tracker
struct TestTracker {
    int calls = 0;
    int lastInt = 0;
    double lastDouble = 0.0;
    std::string lastString;

    void Reset() {
        calls = 0;
        lastInt = 0;
        lastDouble = 0.0;
        lastString.clear();
    }
};

class ReceiverA {
public:
    void HandleInt(int v) {
        tracker.calls++;
        tracker.lastInt = v;
    }

    void HandleConst(int v) const {
        tracker.calls++;
        tracker.lastInt = v;
    }

    void HandleMulti(int a, double b, const std::string& c) {
        tracker.calls++;
        tracker.lastInt = a;
        tracker.lastDouble = b;
        tracker.lastString = c;
    }

    void HandleConstMulti(int a, double b, const std::string& c) const {
        tracker.calls++;
        tracker.lastInt = a;
        tracker.lastDouble = b;
        tracker.lastString = c;
    }

    mutable TestTracker tracker;
};

class ReceiverB {
public:
    void HandleBound(int a, const std::string& b) {
        tracker.calls++;
        tracker.lastInt = a;
        tracker.lastString = b;
    }

    TestTracker tracker;
};

static int g_freeFuncCalls = 0;
static int g_freeFuncLastVal = 0;
void FreeFunction(int val) {
    g_freeFuncCalls++;
    g_freeFuncLastVal = val;
}

void TestBroadcastEvent() {
    std::cout << "Running TestBroadcastEvent...\n";
    FunctionSpeaker::MultiCastDelegate<int> onInt;
    ReceiverA receiver;

    assert(onInt.Empty());
    assert(onInt.Size() == 0);
    assert(!onInt);

    auto h1 = onInt.Add(&receiver, &ReceiverA::HandleInt);
    assert(!onInt.Empty());
    assert(onInt.Size() == 1);
    assert(onInt.Contains(h1));
    assert(onInt.Contains(&receiver));
    assert(onInt);

    onInt.ExecuteAll(42);
    assert(receiver.tracker.calls == 1);
    assert(receiver.tracker.lastInt == 42);

    // Call operator alias
    onInt(100);
    assert(receiver.tracker.calls == 2);
    assert(receiver.tracker.lastInt == 100);

    // Test remove by handle
    bool removed = onInt.Remove(h1);
    assert(removed);
    (void)removed;
    assert(onInt.Empty());
    onInt(200);
    assert(receiver.tracker.calls == 2); // Not called

    std::cout << "TestBroadcastEvent passed.\n";
}

void TestConstMemberAndMultipleArgs() {
    std::cout << "Running TestConstMemberAndMultipleArgs...\n";
    FunctionSpeaker::MultiCastDelegate<int, double, std::string> onMulti;
    const ReceiverA constReceiver;
    ReceiverA nonConstReceiver;

    onMulti.Add(&constReceiver, &ReceiverA::HandleConstMulti);
    onMulti.Add(&nonConstReceiver, &ReceiverA::HandleMulti);

    onMulti.ExecuteAll(10, 2.718, "test");
    assert(constReceiver.tracker.calls == 1);
    assert(constReceiver.tracker.lastInt == 10);
    assert(constReceiver.tracker.lastDouble == 2.718);
    assert(constReceiver.tracker.lastString == "test");
    assert(nonConstReceiver.tracker.calls == 1);
    assert(nonConstReceiver.tracker.lastInt == 10);
    assert(nonConstReceiver.tracker.lastDouble == 2.718);
    assert(nonConstReceiver.tracker.lastString == "test");

    // Also test const member on a single-argument delegate
    FunctionSpeaker::MultiCastDelegate<int> onInt;
    onInt.Add(&constReceiver, &ReceiverA::HandleConst);
    onInt(500);
    assert(constReceiver.tracker.calls == 2);
    assert(constReceiver.tracker.lastInt == 500);

    std::cout << "TestConstMemberAndMultipleArgs passed.\n";
}

void TestLambdasAndFreeFunctions() {
    std::cout << "Running TestLambdasAndFreeFunctions...\n";
    FunctionSpeaker::MultiCastDelegate<int> onInt;

    g_freeFuncCalls = 0;
    g_freeFuncLastVal = 0;

    int lambdaCalls = 0;
    int lambdaLastVal = 0;

    auto hFree = onInt.Add(&FreeFunction);
    auto hLambda = onInt.Add([&](int val) {
        lambdaCalls++;
        lambdaLastVal = val;
    });

    onInt(99);
    assert(g_freeFuncCalls == 1);
    assert(g_freeFuncLastVal == 99);
    assert(lambdaCalls == 1);
    assert(lambdaLastVal == 99);

    onInt.Remove(hFree);
    onInt(123);
    assert(g_freeFuncCalls == 1);
    assert(lambdaCalls == 2);
    assert(lambdaLastVal == 123);

    onInt.Remove(hLambda);
    assert(onInt.Empty());

    std::cout << "TestLambdasAndFreeFunctions passed.\n";
}

void TestBoundArgumentsZeroArgDelegate() {
    std::cout << "Running TestBoundArgumentsZeroArgDelegate...\n";
    // Using CTAD with no template parameters (FunctionSpeaker::MultiCastDelegate myDelegate;)
    FunctionSpeaker::MultiCastDelegate myDelegate;
    ReceiverA a;
    ReceiverB b;

    myDelegate.Add(&a, &ReceiverA::HandleInt, 77);
    myDelegate.Add(&b, &ReceiverB::HandleBound, 88, std::string("bound_message"));

    int lambdaRun = 0;
    myDelegate.Add([&]() { lambdaRun++; });

    myDelegate.ExecuteAll();

    assert(a.tracker.calls == 1);
    assert(a.tracker.lastInt == 77);
    assert(b.tracker.calls == 1);
    assert(b.tracker.lastInt == 88);
    assert(b.tracker.lastString == "bound_message");
    assert(lambdaRun == 1);

    std::cout << "TestBoundArgumentsZeroArgDelegate passed.\n";
}

void TestRemoveByInstance() {
    std::cout << "Running TestRemoveByInstance...\n";
    FunctionSpeaker::MultiCastDelegate<int> onInt;
    ReceiverA a1;
    ReceiverA a2;

    onInt.Add(&a1, &ReceiverA::HandleInt);
    onInt.Add(&a1, &ReceiverA::HandleConst);
    onInt.Add(&a2, &ReceiverA::HandleInt);

    assert(onInt.Size() == 3);

    // Remove all for a1
    std::size_t removed = onInt.Remove(&a1);
    assert(removed == 2);
    (void)removed;
    assert(onInt.Size() == 1);
    assert(!onInt.Contains(&a1));
    assert(onInt.Contains(&a2));

    onInt(55);
    assert(a1.tracker.calls == 0);
    assert(a2.tracker.calls == 1);
    assert(a2.tracker.lastInt == 55);

    std::cout << "TestRemoveByInstance passed.\n";
}

void TestRuleOfFiveCopyAndMove() {
    std::cout << "Running TestRuleOfFiveCopyAndMove...\n";
    FunctionSpeaker::MultiCastDelegate<int> original;
    ReceiverA a;
    original.Add(&a, &ReceiverA::HandleInt);

    // Copy construction
    FunctionSpeaker::MultiCastDelegate<int> copyConstructed = original;
    assert(copyConstructed.Size() == 1);

    // Trigger original
    original(10);
    assert(a.tracker.calls == 1);

    // Trigger copy
    copyConstructed(20);
    assert(a.tracker.calls == 2);
    assert(a.tracker.lastInt == 20);

    // Modifying copy does not modify original
    copyConstructed.Clear();
    assert(copyConstructed.Empty());
    assert(original.Size() == 1);

    // Move construction
    FunctionSpeaker::MultiCastDelegate<int> movedTo = std::move(original);
    assert(movedTo.Size() == 1);
    movedTo(30);
    assert(a.tracker.calls == 3);
    assert(a.tracker.lastInt == 30);

    std::cout << "TestRuleOfFiveCopyAndMove passed.\n";
}

void TestReentrancySafety() {
    std::cout << "Running TestReentrancySafety...\n";
    FunctionSpeaker::MultiCastDelegate<int> delegate;

    FunctionSpeaker::DelegateHandle h1 = FunctionSpeaker::InvalidHandle;
    FunctionSpeaker::DelegateHandle h2 = FunctionSpeaker::InvalidHandle;
    int c1 = 0;
    int c2 = 0;
    int cAdded = 0;

    // Delegate 1 removes Delegate 2 and adds a new delegate
    h1 = delegate.Add([&](int val) {
        c1 += val;
        delegate.Remove(h2);
        delegate.Add([&](int v) { cAdded += v; });
    });

    h2 = delegate.Add([&](int val) {
        c2 += val;
    });

    delegate(10);

    // In the first run, h1 runs, removes h2 before it executes, and adds cAdded for next round
    assert(c1 == 10);
    assert(c2 == 0); // h2 was removed before execution
    assert(cAdded == 0); // newly added delegate should not run in current execution snapshot

    // Second run
    delegate(5);
    assert(c1 == 15);
    assert(cAdded == 5); // now newly added delegate executes

    std::cout << "TestReentrancySafety passed.\n";
}

void TestFunctionSignatureSpecialization() {
    std::cout << "Running TestFunctionSignatureSpecialization...\n";
    // MultiCastDelegate<void(int)> syntax like std::function
    FunctionSpeaker::MultiCastDelegate<void(int)> onHealth;
    int valReceived = 0;
    onHealth.Add([&](int hp) { valReceived = hp; });
    onHealth(75);
    assert(valReceived == 75);

    std::cout << "TestFunctionSignatureSpecialization passed.\n";
}

void TestVersion() {
    std::cout << "Running TestVersion...\n";
    const char* v = FunctionSpeaker::GetVersion();
    assert(v != nullptr);
    assert(std::string(v) == "0.2.0");
    std::cout << "TestVersion passed (version: " << v << ").\n";
}

int main() {
    std::cout << "=== Running FunctionSpeaker Tests ===\n";
    TestBroadcastEvent();
    TestConstMemberAndMultipleArgs();
    TestLambdasAndFreeFunctions();
    TestBoundArgumentsZeroArgDelegate();
    TestRemoveByInstance();
    TestRuleOfFiveCopyAndMove();
    TestReentrancySafety();
    TestFunctionSignatureSpecialization();
    TestVersion();
    std::cout << "=== All FunctionSpeaker Tests Passed! ===\n";
    return 0;
}
