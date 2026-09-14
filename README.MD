# FunctionSpeaker

FunctionSpeaker is a lightweight, header-centric modern C++20 library providing a flexible, memory-safe, and type-safe implementation of the multicast delegate / observer pattern. It enables decoupled, event-driven architectures for games, applications, and embedded systems.

## Features

- **Type-Safe Multicast Delegates**: Support for both broadcast payloads (`MultiCastDelegate<Args...>`) and bound arguments (`MultiCastDelegate<>` or CTAD).
- **Universal Callable Support**:
  - Member functions (both non-const and `const`)
  - Free functions and static methods
  - Lambdas and functors (stateless or capturing)
- **Memory Safety & Value Semantics**:
  - Fully compliant with the **Rule of Five** (no raw-pointer leaks, safe deep-copying, and `noexcept` moving).
  - Eliminates double-free and use-after-free pitfalls.
- **Reentrancy Safety**:
  - Callbacks can safely add or remove listeners during execution without iterator invalidation.
- **Lifecycle Management**:
  - Unsubscribe by subscription handle: `delegate.Remove(handle)` or `delegate -= handle`.
  - Unsubscribe by instance pointer: `delegate.Remove(&instance)` or `delegate -= &instance`.
  - Check membership: `delegate.Contains(handle)`, `delegate.Contains(&instance)`.
  - Inspect state: `delegate.Empty()`, `delegate.Size()`, and `if (delegate)`.
- **Modern C++20**:
  - Built with `std::invoke`, `std::apply`, and C++20 concepts/type constraints for clear compiler diagnostics.
  - Supports function signature syntax: `MultiCastDelegate<void(int, double)>`.
- **Operator Overloads**:
  - `operator()` for invocation: `onEvent(arg1, arg2);`
  - `operator+=` to register callbacks: `onEvent += []() { ... };`
  - `operator-=` to unregister callbacks.

---

## Installation & Integration

### Header-Only / Include

FunctionSpeaker is contained within `FunctionSpeaker.h`. Simply include the header:

```cpp
#include "FunctionSpeaker.h"
```

### CMake Integration

Using CMake `FetchContent` or `add_subdirectory`:

```cmake
add_subdirectory(path/to/FunctionSpeaker)
target_link_libraries(YourProject PRIVATE FunctionSpeaker::FunctionSpeaker)
```

---

## Usage Patterns

### Pattern 1: Typed Broadcast Events (Observer Pattern)

Publishers declare an event with argument types and broadcast data to all subscribers:

```cpp
#include "FunctionSpeaker.h"
#include <iostream>

class Player {
public:
    Player(int initialHealth) : health(initialHealth) {}

    void TakeDamage(int damage) {
        health = std::max(0, health - damage);
        onHealthChanged(health); // Invokes all listeners with the new health
    }

    // Broadcast delegate taking (int currentHealth)
    FunctionSpeaker::MultiCastDelegate<int> onHealthChanged;

private:
    int health;
};

class HealthUI {
public:
    void UpdateHealth(int newHealth) const {
        std::cout << "UI: Health is now " << newHealth << " HP\n";
    }
};

int main() {
    Player player(100);
    HealthUI healthUI;

    // Register a const member function
    auto handle = player.onHealthChanged.Add(&healthUI, &HealthUI::UpdateHealth);

    // Register a lambda
    player.onHealthChanged.Add([](int hp) {
        std::cout << "Audio: Played heartbeat sound for HP: " << hp << '\n';
    });

    // Trigger events
    player.TakeDamage(30);

    // Unsubscribe when done
    player.onHealthChanged.Remove(handle);
    player.TakeDamage(20);

    return 0;
}
```

---

### Pattern 2: MultiCastDelegate with Bound Arguments (Command / Action Pattern)

Callbacks can have their arguments bound at registration time:

```cpp
#include "FunctionSpeaker.h"
#include <iostream>

class AudioSystem {
public:
    void PlaySound(const std::string& soundName, float volume) {
        std::cout << "Playing " << soundName << " at volume " << volume << '\n';
    }
};

int main() {
    AudioSystem audio;
    FunctionSpeaker::MultiCastDelegate onGameStart;

    // Arguments bound at registration time
    onGameStart.Add(&audio, &AudioSystem::PlaySound, "intro_fanfare.wav", 1.0f);
    onGameStart.Add([]() {
        std::cout << "Game started!\n";
    });

    // Execute all registered actions
    onGameStart.ExecuteAll();

    return 0;
}
```

---

## API Overview

### Class Template

```cpp
namespace FunctionSpeaker {
    template<typename... Args>
    class MultiCastDelegate;
}
```

### Registration Methods

```cpp
// Member function (supports non-const and const methods)
Handle Add(T* instance, MemberFunc func, BoundArgs&&... bound);

// Free function, lambda, or functor
Handle Add(Callable&& callable, BoundArgs&&... bound);

// Callable tied to an instance pointer for lifecycle tracking
Handle AddWithContext(const T* instance, Callable&& callable, BoundArgs&&... bound);

// Operator += shortcut
Handle operator+=(Callable&& callable);
```

### Removal Methods

```cpp
// Remove by handle
bool Remove(Handle handle);
bool operator-=(Handle handle);

// Remove all callbacks associated with an instance pointer
std::size_t Remove(const void* instance);
std::size_t operator-=(const void* instance);

// Remove all callbacks
void Clear() noexcept;
```

### Execution Methods

```cpp
// Execute all registered callbacks
void ExecuteAll(Args... args) const;

// Call operator overload
void operator()(Args... args) const;
```

### State Inspection

```cpp
bool Empty() const noexcept;
std::size_t Size() const noexcept;
bool Contains(Handle handle) const noexcept;
bool Contains(const void* instance) const noexcept;
explicit operator bool() const noexcept;
```

---

## Building and Testing

```bash
# Configure with tests and examples enabled
cmake -B build

# Build Release binaries
cmake --build build --config Release

# Run automated tests
ctest --test-dir build -C Release --output-on-failure

# Run the example
./build/Release/FunctionSpeakerExample
```

## License

This project is licensed under the MIT License - see the `LICENSE.txt` file for details.
