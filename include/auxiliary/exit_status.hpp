#pragma once

#include <optional>

enum class exit_status {
    Success, Failure
};

inline exit_status operator&&(exit_status s1, exit_status s2) {
    return (s1 == exit_status::Success && s2 == exit_status::Success) ?
          exit_status::Success
        : exit_status::Failure;
}

inline exit_status exit_status_of(bool b) {
    return b ? exit_status::Success : exit_status::Failure;
}

template<typename T>
inline std::optional<T> optional_of(exit_status s, T&& x) {
    return (s == exit_status::Success) ?
          std::optional<T>(std::forward<T>(x))
        : std::nullopt;
}
