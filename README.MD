# FunctionSpeaker

FunctionSpeaker is a lightweight C++ library that provides a flexible and type-safe implementation of the multicast delegate pattern. It allows you to easily create event-driven systems in your C++ applications.

## Features

- Type-safe delegate system
- Support for member functions with multiple arguments
- Easy-to-use multicast delegate for invoking multiple callbacks

## Installation

To use FunctionSpeaker in your project, simply include the `FunctionSpeaker.h` header file in your C++ project.

```cpp
#include "FunctionSpeaker.h"
```

## Usage

Here's a simple example demonstrating how to use FunctionSpeaker with a health system in a game:

```cpp
#include "FunctionSpeaker.h"
#include <iostream>

// Player class with health
class Player {
public:
    Player(int initialHealth) : health(initialHealth) {}

    void TakeDamage(int damage) {
        health -= damage;
        if (health < 0) health = 0;
        onHealthChanged.ExecuteAll(health);
    }

    void Heal(int amount) {
        health += amount;
        if (health > 100) health = 100;
        onHealthChanged.ExecuteAll(health);
    }

    int GetHealth() const { return health; }

    MultiCastDelegate<int> onHealthChanged;

private:
    int health;
};

// UI class to display health
class HealthUI {
public:
    void UpdateHealth(int newHealth) {
        std::cout << "Health UI updated. New health: " << newHealth << std::endl;
    }
};

int main() {
    Player player(100);
    HealthUI healthUI;

    // Register the UpdateHealth function to be called when health changes
    player.onHealthChanged.Add(&healthUI, &HealthUI::UpdateHealth);

    // Simulate game events
    std::cout << "Player takes 30 damage" << std::endl;
    player.TakeDamage(30);

    std::cout << "Player heals 20 health" << std::endl;
    player.Heal(20);

    return 0;
}
```

This example demonstrates how to:
1. Create a `Player` class with health and an `onHealthChanged` delegate.
2. Create a `HealthUI` class with an `UpdateHealth` method.
3. Register the `UpdateHealth` method to be called when the player's health changes.
4. Trigger health changes and observe the automatic UI updates.

## API Overview

### MultiCastDelegate

The main class you'll interact with. It allows you to register multiple callbacks and execute them all at once.

```cpp
MultiCastDelegate<Args...> myDelegate;
```

#### Adding a callback

```cpp
myDelegate.Add(instance, &Class::Method, args...);
```

#### Executing all callbacks

```cpp
myDelegate.ExecuteAll(args...);
```
