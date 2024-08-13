#include "FunctionSpeaker.h"
#include <iostream>

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

int main() {
    MyClassA a;
    MyClassB b;
    MyClassC c;

    MultiCastDelegate myDelegate;

    // Convert string literal to std::string explicitly
    myDelegate.Add(&a, &MyClassA::OnEvent, 42);
    myDelegate.Add(&b, &MyClassB::OnEvent, "Hello, World!");
    myDelegate.Add(&c, &MyClassC::OnEvent, 7, 3.14);

    // Simulate an event that triggers the callbacks
    std::cout << "Executing all registered callbacks:\n";
    myDelegate.ExecuteAll();

    std::cin.get();

    return 0;
}