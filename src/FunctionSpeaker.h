#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace FunctionSpeaker {

using DelegateHandle = std::uint64_t;
inline constexpr DelegateHandle InvalidHandle = 0;

/**
 * @brief Returns the version string of the FunctionSpeaker library.
 */
const char* GetVersion() noexcept;

/**
 * @brief Non-templated base interface for all delegates.
 */
class DelegateBase {
public:
    virtual ~DelegateBase() = default;
};

/**
 * @brief Templated delegate invocation interface.
 * @tparam Args Argument types passed when invoking the delegate.
 */
template<typename... Args>
class IDelegate : public DelegateBase {
public:
    ~IDelegate() override = default;
    virtual void Execute(Args... args) = 0;
    virtual std::unique_ptr<IDelegate<Args...>> Clone() const = 0;
};

namespace detail {

template<typename... BoundArgs>
using DecayTuple = std::tuple<std::decay_t<BoundArgs>...>;

template<typename... Ts>
struct IsInstanceAndMemberPair : std::false_type {};

template<typename T, typename Member, typename... Rest>
struct IsInstanceAndMemberPair<T, Member, Rest...>
    : std::bool_constant<std::is_pointer_v<std::decay_t<T>> && std::is_member_function_pointer_v<std::decay_t<Member>>> {};

template<typename Signature, typename T, typename MemberFunc, typename... BoundArgs>
class MemberDelegate;

template<typename... Args, typename T, typename MemberFunc, typename... BoundArgs>
class MemberDelegate<void(Args...), T, MemberFunc, BoundArgs...> : public IDelegate<Args...> {
public:
    template<typename... UBoundArgs>
    MemberDelegate(T* instance, MemberFunc func, UBoundArgs&&... bound)
        : m_instance(instance), m_func(func), m_boundArgs(std::forward<UBoundArgs>(bound)...) {}

    void Execute(Args... args) override {
        if (m_instance != nullptr && m_func != nullptr) {
            std::apply([&](auto&&... bound) {
                if constexpr (std::is_invocable_v<MemberFunc, T*, Args..., decltype(bound)...>) {
                    std::invoke(m_func, m_instance, args..., bound...);
                } else if constexpr (std::is_invocable_v<MemberFunc, T*, decltype(bound)...>) {
                    std::invoke(m_func, m_instance, bound...);
                } else if constexpr (std::is_invocable_v<MemberFunc, T*, Args...>) {
                    std::invoke(m_func, m_instance, args...);
                } else if constexpr (std::is_invocable_v<MemberFunc, T*>) {
                    std::invoke(m_func, m_instance);
                }
            }, m_boundArgs);
        }
    }

    std::unique_ptr<IDelegate<Args...>> Clone() const override {
        return std::make_unique<MemberDelegate>(*this);
    }

private:
    T* m_instance;
    MemberFunc m_func;
    DecayTuple<BoundArgs...> m_boundArgs;
};

template<typename Signature, typename Callable, typename... BoundArgs>
class CallableDelegate;

template<typename... Args, typename Callable, typename... BoundArgs>
class CallableDelegate<void(Args...), Callable, BoundArgs...> : public IDelegate<Args...> {
public:
    template<typename UCallable, typename... UBoundArgs>
    CallableDelegate(UCallable&& callable, UBoundArgs&&... bound)
        : m_callable(std::forward<UCallable>(callable)),
          m_boundArgs(std::forward<UBoundArgs>(bound)...) {}

    void Execute(Args... args) override {
        std::apply([&](auto&&... bound) {
            if constexpr (std::is_invocable_v<Callable, Args..., decltype(bound)...>) {
                std::invoke(m_callable, args..., bound...);
            } else if constexpr (std::is_invocable_v<Callable, decltype(bound)...>) {
                std::invoke(m_callable, bound...);
            } else if constexpr (std::is_invocable_v<Callable, Args...>) {
                std::invoke(m_callable, args...);
            } else if constexpr (std::is_invocable_v<Callable>) {
                std::invoke(m_callable);
            }
        }, m_boundArgs);
    }

    std::unique_ptr<IDelegate<Args...>> Clone() const override {
        return std::make_unique<CallableDelegate>(*this);
    }

private:
    std::decay_t<Callable> m_callable;
    DecayTuple<BoundArgs...> m_boundArgs;
};

} // namespace detail

/**
 * @brief Thread-friendly, reentrancy-safe, type-safe multicast delegate.
 *
 * Supports member functions (const and non-const), free functions, lambdas, and functors.
 * Supports both broadcast arguments (passed to ExecuteAll) and bound arguments (bound at Add).
 * Fully compliant with the Rule of Five (memory-safe, deep-copyable, and movable).
 *
 * @tparam Args Parameter types passed when broadcasting the event.
 */
template<typename... Args>
class MultiCastDelegate {
public:
    using Handle = DelegateHandle;

    MultiCastDelegate() = default;
    ~MultiCastDelegate() = default;

    // Rule of Five: Deep copy semantics
    MultiCastDelegate(const MultiCastDelegate& other)
        : m_nextId(other.m_nextId) {
        m_delegates.reserve(other.m_delegates.size());
        for (const auto& entry : other.m_delegates) {
            m_delegates.push_back(Entry{
                entry.Id,
                entry.Instance,
                entry.Invoker ? entry.Invoker->Clone() : nullptr
            });
        }
    }

    MultiCastDelegate& operator=(const MultiCastDelegate& other) {
        if (this != &other) {
            MultiCastDelegate copy(other);
            *this = std::move(copy);
        }
        return *this;
    }

    MultiCastDelegate(MultiCastDelegate&&) noexcept = default;
    MultiCastDelegate& operator=(MultiCastDelegate&&) noexcept = default;

    /**
     * @brief Registers a member function with an object instance and optional bound arguments.
     * @param instance Pointer to the class instance (can be const or non-const).
     * @param func Pointer to the member function.
     * @param bound Additional arguments to bind to the function call.
     * @return A DelegateHandle that can be used to unregister the callback.
     */
    template<typename T, typename MemberFunc, typename... BoundArgs>
    requires std::is_member_function_pointer_v<std::decay_t<MemberFunc>> &&
             (std::is_invocable_v<MemberFunc, T*, Args..., std::decay_t<BoundArgs>...> ||
              std::is_invocable_v<MemberFunc, T*, std::decay_t<BoundArgs>...> ||
              std::is_invocable_v<MemberFunc, T*, Args...> ||
              std::is_invocable_v<MemberFunc, T*>)
    Handle Add(T* instance, MemberFunc func, BoundArgs&&... bound) {
        if (!instance || !func) {
            return InvalidHandle;
        }
        const Handle id = GenerateId();
        using InvokerType = detail::MemberDelegate<void(Args...), T, std::decay_t<MemberFunc>, std::decay_t<BoundArgs>...>;
        m_delegates.push_back(Entry{
            id,
            static_cast<const void*>(instance),
            std::make_shared<InvokerType>(instance, func, std::forward<BoundArgs>(bound)...)
        });
        return id;
    }

    /**
     * @brief Registers a free function, lambda, or general callable with optional bound arguments.
     * @param callable The callable object to register.
     * @param bound Additional arguments to bind to the function call.
     * @return A DelegateHandle that can be used to unregister the callback.
     */
    template<typename Callable, typename... BoundArgs>
    requires (!std::is_member_function_pointer_v<std::decay_t<Callable>>) &&
             (!detail::IsInstanceAndMemberPair<Callable, BoundArgs...>::value) &&
             (std::is_invocable_v<Callable, Args..., std::decay_t<BoundArgs>...> ||
              std::is_invocable_v<Callable, std::decay_t<BoundArgs>...> ||
              std::is_invocable_v<Callable, Args...> ||
              std::is_invocable_v<Callable>)
    Handle Add(Callable&& callable, BoundArgs&&... bound) {
        const Handle id = GenerateId();
        using InvokerType = detail::CallableDelegate<void(Args...), std::decay_t<Callable>, std::decay_t<BoundArgs>...>;
        m_delegates.push_back(Entry{
            id,
            nullptr,
            std::make_shared<InvokerType>(std::forward<Callable>(callable), std::forward<BoundArgs>(bound)...)
        });
        return id;
    }

    /**
     * @brief Registers a callable associated with an instance context.
     * Allows Remove(instance) to unregister this callable when the instance is destroyed.
     */
    template<typename T, typename Callable, typename... BoundArgs>
    requires (!std::is_member_function_pointer_v<std::decay_t<Callable>>)
    Handle AddWithContext(const T* instance, Callable&& callable, BoundArgs&&... bound) {
        const Handle id = GenerateId();
        using InvokerType = detail::CallableDelegate<void(Args...), std::decay_t<Callable>, std::decay_t<BoundArgs>...>;
        m_delegates.push_back(Entry{
            id,
            static_cast<const void*>(instance),
            std::make_shared<InvokerType>(std::forward<Callable>(callable), std::forward<BoundArgs>(bound)...)
        });
        return id;
    }

    /**
     * @brief Unregisters a callback using its handle.
     * @return True if a callback was removed, false otherwise.
     */
    bool Remove(Handle handle) {
        if (handle == InvalidHandle) {
            return false;
        }
        auto it = std::remove_if(m_delegates.begin(), m_delegates.end(),
            [handle](const Entry& e) { return e.Id == handle; });
        if (it != m_delegates.end()) {
            m_delegates.erase(it, m_delegates.end());
            return true;
        }
        return false;
    }

    /**
     * @brief Unregisters all callbacks associated with the given instance pointer.
     * @return The number of callbacks removed.
     */
    std::size_t Remove(const void* instance) {
        if (instance == nullptr) {
            return 0;
        }
        const auto origSize = m_delegates.size();
        auto it = std::remove_if(m_delegates.begin(), m_delegates.end(),
            [instance](const Entry& e) { return e.Instance == instance; });
        m_delegates.erase(it, m_delegates.end());
        return origSize - m_delegates.size();
    }

    /**
     * @brief Unregisters all callbacks associated with the given instance pointer.
     */
    template<typename T>
    std::size_t Remove(const T* instance) {
        return Remove(static_cast<const void*>(instance));
    }

    /**
     * @brief Removes all registered callbacks.
     */
    void Clear() noexcept {
        m_delegates.clear();
    }

    /**
     * @brief Checks if any callbacks are registered.
     */
    [[nodiscard]] bool Empty() const noexcept {
        return m_delegates.empty();
    }

    /**
     * @brief Returns the count of registered callbacks.
     */
    [[nodiscard]] std::size_t Size() const noexcept {
        return m_delegates.size();
    }

    /**
     * @brief Checks if a callback with the specified handle is registered.
     */
    [[nodiscard]] bool Contains(Handle handle) const noexcept {
        if (handle == InvalidHandle) {
            return false;
        }
        return std::any_of(m_delegates.begin(), m_delegates.end(),
            [handle](const Entry& e) { return e.Id == handle; });
    }

    /**
     * @brief Checks if any callbacks are registered for the specified instance.
     */
    [[nodiscard]] bool Contains(const void* instance) const noexcept {
        if (instance == nullptr) {
            return false;
        }
        return std::any_of(m_delegates.begin(), m_delegates.end(),
            [instance](const Entry& e) { return e.Instance == instance; });
    }

    template<typename T>
    [[nodiscard]] bool Contains(const T* instance) const noexcept {
        return Contains(static_cast<const void*>(instance));
    }

    /**
     * @brief Executes all registered callbacks with the provided arguments.
     * Reentrancy-safe: callbacks can add or remove delegates without causing iterator invalidation.
     */
    void ExecuteAll(Args... args) const {
        if (m_delegates.empty()) {
            return;
        }
        // Create a snapshot copy so modifications during execution do not invalidate iteration
        auto snapshot = m_delegates;
        for (const auto& entry : snapshot) {
            if (entry.Invoker && Contains(entry.Id)) {
                entry.Invoker->Execute(args...);
            }
        }
    }

    /**
     * @brief Operator () alias for ExecuteAll.
     */
    void operator()(Args... args) const {
        ExecuteAll(std::forward<Args>(args)...);
    }

    /**
     * @brief Returns true if there are registered callbacks.
     */
    explicit operator bool() const noexcept {
        return !m_delegates.empty();
    }

    /**
     * @brief Operator += to add a callable.
     */
    template<typename Callable>
    Handle operator+=(Callable&& callable) {
        return Add(std::forward<Callable>(callable));
    }

    /**
     * @brief Operator -= to remove by handle.
     */
    bool operator-=(Handle handle) {
        return Remove(handle);
    }

    /**
     * @brief Operator -= to remove by instance.
     */
    template<typename T>
    std::size_t operator-=(const T* instance) {
        return Remove(instance);
    }

private:
    struct Entry {
        Handle Id{InvalidHandle};
        const void* Instance{nullptr};
        std::shared_ptr<IDelegate<Args...>> Invoker;
    };

    Handle GenerateId() noexcept {
        return m_nextId++;
    }

    std::vector<Entry> m_delegates;
    Handle m_nextId{1};
};

// Deduction guide for MultiCastDelegate without template arguments: MultiCastDelegate myDelegate;
MultiCastDelegate() -> MultiCastDelegate<>;

// Specialization for function signature syntax, e.g. MultiCastDelegate<void(int, float)>
template<typename Ret, typename... Args>
class MultiCastDelegate<Ret(Args...)> : public MultiCastDelegate<Args...> {
public:
    using MultiCastDelegate<Args...>::MultiCastDelegate;
};

// Legacy compatibility template
template<typename T, typename... Args>
class Delegate : public IDelegate<> {
public:
    using FunctionType = void(T::*)(Args...);

    template<typename... UArgs>
    Delegate(T* instance, FunctionType function, UArgs&&... args)
        : m_impl(instance, function, std::forward<UArgs>(args)...) {}

    void Execute() override {
        m_impl.Execute();
    }

    std::unique_ptr<IDelegate<>> Clone() const override {
        return std::make_unique<Delegate>(*this);
    }

private:
    detail::MemberDelegate<void(), T, FunctionType, Args...> m_impl;
};

} // namespace FunctionSpeaker

// Global namespace aliases for backward compatibility
using FunctionSpeaker::MultiCastDelegate;
using FunctionSpeaker::DelegateBase;
using FunctionSpeaker::Delegate;
using FunctionSpeaker::DelegateHandle;
using FunctionSpeaker::InvalidHandle;