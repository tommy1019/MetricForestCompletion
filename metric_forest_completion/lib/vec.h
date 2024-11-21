#pragma once

#include <array>
#include <sstream>

// General purpose n dimensional templated vector type

template <typename T, size_t N>
struct Vec {
    using Type = T;

#define CONVENIENCE_ACCESSOR(name, index)                                                                                                                                                              \
    constexpr inline T& name() requires(N - 1 >= index)                                                                                                                                                \
    {                                                                                                                                                                                                  \
        return std::get<index>(array);                                                                                                                                                                 \
    }                                                                                                                                                                                                  \
    constexpr inline const T& name() const requires(N - 1 >= index)                                                                                                                                    \
    {                                                                                                                                                                                                  \
        return std::get<index>(array);                                                                                                                                                                 \
    }

    CONVENIENCE_ACCESSOR(x, 0);
    CONVENIENCE_ACCESSOR(y, 1);
    CONVENIENCE_ACCESSOR(z, 2);
    CONVENIENCE_ACCESSOR(w, 3);
    CONVENIENCE_ACCESSOR(v, 4);
#undef CONVENIENCE_ACCESSOR

    constexpr T length_squared() const {
        T val = 0;
        const_for<N>([&]<size_t I>() { val += std::get<I>(array) * std::get<I>(array); });
        return val;
    }

    constexpr T length() const { return std::sqrt(length_squared()); }

    constexpr T l1() const {
        T val = 0;
        for (auto& v : array)
            val += std::abs(v);
        return val;
    }

    constexpr static inline void dot(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
        T res = 0;
        const_for<N>([&]<size_t I>() { res += std::get<I>(lhs.array) * std::get<I>(rhs.array); });
        return res;
    }

    auto operator<=>(const Vec<T, N>&) const = default;

    constexpr static inline void add(Vec<T, N>& res, const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
        const_for<N>([&]<size_t I>() { std::get<I>(res.array) = std::get<I>(lhs.array) + std::get<I>(rhs.array); });
    }

    constexpr static inline void sub(Vec<T, N>& res, const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
        const_for<N>([&]<size_t I>() { std::get<I>(res.array) = std::get<I>(lhs.array) - std::get<I>(rhs.array); });
    }

    constexpr static inline void mul(Vec<T, N>& res, const Vec<T, N>& lhs, const T& rhs) {
        const_for<N>([&]<size_t I>() { std::get<I>(res.array) = std::get<I>(lhs.array) * rhs; });
    }
    constexpr static inline void mul(Vec<T, N>& res, const T& lhs, const Vec<T, N>& rhs) {
        const_for<N>([&]<size_t I>() { std::get<I>(res.array) = lhs * std::get<I>(rhs.array); });
    }

    constexpr static inline void div(Vec<T, N>& res, const Vec<T, N>& lhs, const T& rhs) {
        const_for<N>([&]<size_t I>() { std::get<I>(res.array) = std::get<I>(lhs.array) / rhs; });
    }
    constexpr static inline void div(Vec<T, N>& res, const T& lhs, const Vec<T, N>& rhs) {
        const_for<N>([&]<size_t I>() { std::get<I>(res.array) = lhs / std::get<I>(rhs.array); });
    }

    constexpr inline friend const Vec<T, N> operator+(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
        Vec<T, N> res;
        add(res, lhs, rhs);
        return res;
    }
    constexpr inline friend const Vec<T, N> operator-(const Vec<T, N>& lhs, const Vec<T, N>& rhs) {
        Vec<T, N> res;
        sub(res, lhs, rhs);
        return res;
    }
    constexpr inline friend const Vec<T, N> operator*(const Vec<T, N>& lhs, const T& rhs) {
        Vec<T, N> res;
        mul(res, lhs, rhs);
        return res;
    }
    constexpr inline friend const Vec<T, N> operator*(const T& lhs, const Vec<T, N>& rhs) {
        Vec<T, N> res;
        mul(res, lhs, rhs);
        return res;
    }
    constexpr inline friend const Vec<T, N> operator/(const Vec<T, N>& lhs, const T& rhs) {
        Vec<T, N> res;
        div(res, lhs, rhs);
        return res;
    }
    constexpr inline friend const Vec<T, N> operator/(const T& lhs, const Vec<T, N>& rhs) {
        Vec<T, N> res;
        div(res, lhs, rhs);
        return res;
    }

    std::array<T, N> array;
};

template <size_t N, size_t I = 0, typename Func>
constexpr void const_for(Func f) {
    if constexpr (I < N) {
        f.template operator()<I>();
        const_for<N, I + 1, Func>(f);
    }
}

template <typename T, size_t N>
struct std::formatter<Vec<T, N>> : std::formatter<T> {
    template <typename FormatContext>
    auto format(const Vec<T, N>& p, FormatContext& ctx) const {
        format_to(ctx.out(), "[");
        if constexpr (N > 0)
            std::formatter<T>::format(p.x(), ctx);
        const_for<N - 1>([&]<size_t I>() {
            format_to(ctx.out(), ", ");
            std::formatter<T>::format(std::get<I + 1>(p.array), ctx);
        });
        return format_to(ctx.out(), "]");
    }
};