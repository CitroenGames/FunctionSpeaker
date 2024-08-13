#include <iostream>
#include <functional>
#include <vector>
#include <tuple>
#include <string>
#include <utility>

class DelegateBase {
public:
    virtual ~DelegateBase() = default;
    virtual void Execute() = 0;
};

template<typename T, typename... Args>
class Delegate : public DelegateBase {
public:
    using FunctionType = void(T::*)(Args...);

    Delegate(T* instance, FunctionType function, Args&&... args)
        : Instance(instance), Function(function), Arguments(std::forward<Args>(args)...) {}

    void Execute() override {
        ExecuteImpl(std::index_sequence_for<Args...>{});
    }

private:
    template<std::size_t... I>
    void ExecuteImpl(std::index_sequence<I...>) {
        (Instance->*Function)(std::get<I>(Arguments)...);
    }

    T* Instance;
    FunctionType Function;
    std::tuple<Args...> Arguments;
};

class MultiCastDelegate {
public:
    template<typename T, typename... Args, typename... ActualArgs>
    void Add(T* instance, void(T::* function)(Args...), ActualArgs&&... args) {
        Delegates.push_back(new Delegate<T, Args...>(instance, function, std::forward<ActualArgs>(args)...));
    }

    void ExecuteAll() {
        for (auto& delegate : Delegates) {
            delegate->Execute();
        }
    }

    ~MultiCastDelegate() {
        for (auto& delegate : Delegates) {
            delete delegate;
        }
        Delegates.clear();
    }

private:
    std::vector<DelegateBase*> Delegates;
};