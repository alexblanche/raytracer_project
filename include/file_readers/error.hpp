#pragma once

namespace file_reader {
    enum class error {
        ReadingErrorHeader, ReadingErrorData,
        WritingErrorHeader, WritingErrorData,
        FileError, Other
    };
}