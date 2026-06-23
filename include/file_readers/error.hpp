#pragma once

namespace file_reader {
    enum class error {
        DataError,
        ReadingErrorHeader, ReadingErrorData,
        WritingErrorHeader, WritingErrorData,
        FileError,
        Other
    };
}