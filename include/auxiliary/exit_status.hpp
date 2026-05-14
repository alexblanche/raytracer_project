#pragma once

enum class exit_status {
    Success, Failure
};

inline exit_status operator&&(exit_status s1, exit_status s2) {
    return (s1 == exit_status::Success && s2 == exit_status::Success) ?
          exit_status::Success
        : exit_status::Failure;
}