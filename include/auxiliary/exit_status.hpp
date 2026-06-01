#pragma once

#include <optional>

enum class exit_status {
    Success, Failure
};

inline exit_status operator&&(exit_status s1, exit_status s2) {
    using enum exit_status;
    return (s1 == Success && s2 == Success) ? Success : Failure;
}

inline exit_status exit_status_of(bool b) {
    using enum exit_status;
    return b ? Success : Failure;
}

template<typename T>
inline std::optional<T> optional_of(exit_status s, T&& x) {
    return (s == exit_status::Success) ?
          std::optional<T>(std::forward<T>(x))
        : std::nullopt;
}
