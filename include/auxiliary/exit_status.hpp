#pragma once

#include <optional>
#include <cstdlib>
#include <stdexcept>

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

inline void exit_if_failure(exit_status s) {
    if (s == exit_status::Failure)
        exit(EXIT_FAILURE);
}

inline void throw_if_failure(exit_status s, const std::string& error_message) {
    if (s == exit_status::Failure)
        throw std::runtime_error(error_message);
}