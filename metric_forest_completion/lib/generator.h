#pragma once

// Implementation of std::generator for cases when the host compiler (clang) has none

#if __has_include(<generator>)
#include <generator>
#else

#include <coroutine>
#include <iterator>
#include <optional>

namespace std {

template <std::movable T>
class generator {
  public:
    struct promise_type {

        generator<T> get_return_object() { return generator{handle::from_promise(*this)}; }

        static std::suspend_always initial_suspend() noexcept { return {}; }

        static std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) noexcept {
            current_value = std::move(value);
            return {};
        }

        void await_transform() = delete;
        [[noreturn]]
        static void unhandled_exception() {
            throw;
        }

        std::optional<T> current_value;
    };

    using handle = std::coroutine_handle<promise_type>;

    explicit generator(const handle coroutine) : m_coroutine{coroutine} {}

    generator() = default;
    ~generator() {
        if (m_coroutine)
            m_coroutine.destroy();
    }

    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;

    generator(generator&& other) noexcept : m_coroutine{other.m_coroutine} { other.m_coroutine = {}; }
    generator& operator=(generator&& other) noexcept {
        if (this != &other) {
            if (m_coroutine)
                m_coroutine.destroy();
            m_coroutine = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    // Range-based for loop support.
    class iterator {
      public:
        void operator++() { m_coroutine.resume(); }
        const T& operator*() const { return *m_coroutine.promise().current_value; }
        bool operator==(std::default_sentinel_t) const { return !m_coroutine || m_coroutine.done(); }

        explicit iterator(const handle coroutine) : m_coroutine{coroutine} {}

      private:
        handle m_coroutine;
    };

    iterator begin() {
        if (m_coroutine)
            m_coroutine.resume();
        return iterator{m_coroutine};
    }

    std::default_sentinel_t end() { return {}; }

  private:
    handle m_coroutine;
};

} // namespace std

#endif