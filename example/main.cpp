#include "FunctionSpeaker.h"
#include <iostream>
#include <string>

// Example classes
class MyClassA {
public:
    void OnEvent(int value) {
        std::cout << "MyClassA::OnEvent called with value: " << value << '\n';
    }
};

class MyClassB {
public:
    void OnEvent(const std::string& message) {
        std::cout << "MyClassB::OnEvent called with message: " << message << '\n';
    }
};

class MyClassC {
public:
    void OnEvent(int x, double y) {
        std::cout << "MyClassC::OnEvent called with x: " << x << " and y: " << y << '\n';
    }
};

class HealthUI {
public:
    void OnHealthChanged(int currentHealth) {
        std::cout << "HealthUI: Updated health display to " << currentHealth << " HP\n";
    }
};

int main() {
    std::cout << "=== Pattern 1: MultiCastDelegate with Bound Arguments ===\n";
    MyClassA a;
    MyClassB b;
    MyClassC c;

    FunctionSpeaker::MultiCastDelegate myDelegate;

    myDelegate.Add(&a, &MyClassA::OnEvent, 42);
    myDelegate.Add(&b, &MyClassB::OnEvent, "Hello, World!");
    myDelegate.Add(&c, &MyClassC::OnEvent, 7, 3.14);

    // Also supports lambdas
    myDelegate.Add([]() {
        std::cout << "Anonymous lambda callback executed!\n";
    });

    std::cout << "Executing all registered callbacks:\n";
    myDelegate.ExecuteAll();

    std::cout << "\n=== Pattern 2: Typed Broadcast Event (Observer Pattern) ===\n";
    FunctionSpeaker::MultiCastDelegate<int> onHealthChanged;
    HealthUI healthUI;

    auto uiHandle = onHealthChanged.Add(&healthUI, &HealthUI::OnHealthChanged);
    onHealthChanged.Add([](int hp) {
        std::cout << "Analytics: Recorded health event: " << hp << " HP\n";
    });

    std::cout << "Broadcasting health change (100 HP):\n";
    onHealthChanged(100);

    std::cout << "Broadcasting health change (75 HP):\n";
    onHealthChanged(75);

    std::cout << "\nRemoving HealthUI listener...\n";
    onHealthChanged.Remove(uiHandle);

    std::cout << "Broadcasting health change (50 HP) after removal:\n";
    onHealthChanged(50);

    std::cout << "\nAll examples completed successfully.\n";
    return 0;
}