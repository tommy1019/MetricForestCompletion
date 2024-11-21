#pragma once

#include <expected>
#include <optional>
#include <string>

// General purpose error class. Used instead of exceptions for functions that may return an error

template <typename T>
using ErrorOr = std::expected<T, std::string>;

template <typename T>
std::optional<T> to_optional(ErrorOr<T> v) {
    if (v.has_value())
        return *v;
    return {};
}

#define COLOR_RED "\x1B[31m"
#define COLOR_GRN "\x1B[32m"
#define COLOR_YEL "\x1B[33m"
#define COLOR_BLU "\x1B[34m"
#define COLOR_MAG "\x1B[35m"
#define COLOR_CYN "\x1B[36m"
#define COLOR_WHT "\x1B[37m"
#define COLOR_RESET "\x1B[0m"

#define ERR(...) std::unexpected(__VA_ARGS__);

#define MAYBE(...) (void)!__VA_ARGS__

#define MUST(...)                                                                                                                                                                                      \
    ({                                                                                                                                                                                                 \
        auto RESULT = __VA_ARGS__;                                                                                                                                                                     \
        if (!RESULT.has_value()) {                                                                                                                                                                     \
            auto error = std::move(RESULT.error());                                                                                                                                                    \
            printf("Error: %s\n", error.c_str());                                                                                                                                                      \
            printf("    The program aborted at line " COLOR_RED "%d" COLOR_RESET " of function " COLOR_YEL "%s" COLOR_RESET " in file " COLOR_CYN "%s" COLOR_RESET "\n",                               \
                   __LINE__,                                                                                                                                                                           \
                   __FUNCTION__,                                                                                                                                                                       \
                   __FILE__);                                                                                                                                                                          \
            fflush(stdout);                                                                                                                                                                            \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
        *std::move(RESULT);                                                                                                                                                                            \
    })

#define TRY(...)                                                                                                                                                                                       \
    ({                                                                                                                                                                                                 \
        auto RESULT = __VA_ARGS__;                                                                                                                                                                     \
        if (!(RESULT).has_value()) {                                                                                                                                                                   \
            return std::unexpected(std::move(RESULT.error()));                                                                                                                                         \
        }                                                                                                                                                                                              \
        *std::move(RESULT);                                                                                                                                                                            \
    })

#define REQUIRE(TEST, MESSAGE, ...)                                                                                                                                                                    \
    {                                                                                                                                                                                                  \
        if (!(TEST)) {                                                                                                                                                                                 \
            printf("Error: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                                                             \
            printf("    The error occurred in function " COLOR_YEL "%s" COLOR_RESET " at " COLOR_CYN "%s" COLOR_RESET ":" COLOR_RED "%d" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);          \
            fflush(stdout);                                                                                                                                                                            \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    }

#define REQUIRE_EQUALS(TEST, VALUE, MESSAGE, ...)                                                                                                                                                      \
    {                                                                                                                                                                                                  \
        auto RESULT = TEST;                                                                                                                                                                            \
        if (RESULT != VALUE) {                                                                                                                                                                         \
            printf("Error: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                                                             \
            printf("    The error occurred in function " COLOR_YEL "%s" COLOR_RESET " at " COLOR_CYN "%s" COLOR_RESET ":" COLOR_RED "%d" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);          \
            fflush(stdout);                                                                                                                                                                            \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    }

#define REQUIRE_NOT_REACHED(MESSAGE, ...)                                                                                                                                                              \
    {                                                                                                                                                                                                  \
        printf("Error: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                                                                 \
        printf("    The error occurred in function " COLOR_YEL "%s" COLOR_RESET " at " COLOR_CYN "%s" COLOR_RESET ":" COLOR_RED "%d" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);              \
        fflush(stdout);                                                                                                                                                                                \
        abort();                                                                                                                                                                                       \
    }

#define REQUIRE_TODO(MESSAGE, ...)                                                                                                                                                                     \
    {                                                                                                                                                                                                  \
        printf("Error: This code path is not written yet: " MESSAGE "\n", ##__VA_ARGS__);                                                                                                              \
        printf("    The error occurred in function " COLOR_YEL "%s" COLOR_RESET " at " COLOR_CYN "%s" COLOR_RESET ":" COLOR_RED "%d" COLOR_RESET "\n", __FUNCTION__, __FILE__, __LINE__);              \
        fflush(stdout);                                                                                                                                                                                \
        abort();                                                                                                                                                                                       \
    }
