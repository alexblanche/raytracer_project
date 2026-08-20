#pragma once

#include "auxiliary/exit_status.hpp"
#include "parameters.hpp"

#include <iostream>
#include <span>
#include <vector>
#include <array>
#include <optional>




constexpr unsigned int INFINITE = static_cast<unsigned int>(-1);
constexpr unsigned int MAX_STRING_LENGTH = 1000;

// Used to ignore unused indices from std::index_sequence
template<typename T>
static consteval T id(std::size_t, T value) {
    return value;
}

// Auxiliary
template<typename T, std::size_t... i>
static consteval std::array<T, sizeof...(i)> make_array_aux_(T value, std::index_sequence<i...>) {
    return { id(i, value)... };
}

// Returns an array with all the elements initialized to value
template<typename T, std::size_t count>
static consteval std::array<T, count> make_array(T value) {
    return make_array_aux_<T>(value, std::make_index_sequence<count>());
}

template<typename T>
concept Arithm = std::is_arithmetic_v<T>;


class file {
    
    enum class mode {
        R, RB, W, WB, A, AB
    };
    static constexpr mode string_of_mode(const std::string& s) {
        using enum mode;
        if (s == "r" ) return R;
        if (s == "rb") return RB;
        if (s == "w" ) return W;
        if (s == "wb") return WB;
        if (s == "a" ) return A;
        if (s == "ab") return AB;
        return R; // default
    }

    public:
        FILE *f;
    private:
        mode mode_;

        static constexpr std::size_t MAX_FORMAT_SIZE = 1000;

        template<std::size_t size>
        class ZString {

            public:
                std::array<char, size + 1> data; // size = max length
                std::size_t length; // logical size

                constexpr ZString() {
                    for (char& c : data)
                        c = '\0';
                    length = 0;
                }

                constexpr ZString(const std::string& s)
                    : ZString() {

                    std::copy(s.begin(), s.end(), data.begin());
                    length = s.length();
                    if (s.length() > size)
                        throw std::runtime_error("ZString: string is too long\n");
                }

                constexpr ZString(const char* zs) : ZString() {
                    std::size_t i = 0;
                    char c = *zs;
                    while (i < size && c != '\0') {
                        data[i++] = c;
                        zs++;
                        c = *zs;
                    }
                    length = i;
                }

                constexpr const char* c_str() const {
                    return data.data();
                }

                template<std::size_t size2>
                constexpr auto operator+(const ZString<size2>& s2) const {
                    return concat(*this, s2);
                }
        };

        template<std::size_t size1, std::size_t size2>
        static constexpr ZString<size1 + size2> concat(const ZString<size1>& s1, const ZString<size2>& s2) {
            
            constexpr std::size_t size_total = size1 + size2;
            ZString<size_total> out;
            for (std::size_t i = 0; i < s1.length; i++) {
                out.data[i] = s1.data[i];
            }
            for (std::size_t i = 0; i < s2.length; i++) {
                out.data[i + s1.length] = s2.data[i];
            }
            out.length = s1.length + s2.length;
            return out;
        }

        template<std::size_t count, std::size_t size1>
        static constexpr ZString<count * size1> concat_count(const ZString<size1>& s) {

            constexpr std::size_t size_total = count * size1;
            ZString<size_total> out;
            for (std::size_t i = 0; i < count; i++) {
                for (std::size_t j = 0; j < s.length; j++) {
                    out.data[i * s.length + j] = s.data[j];
                }
            }
            out.length = count * s.length;
            return out;
        }

        // Returns the printf/scanf format for data type T
        template<Arithm T>
        static constexpr ZString<4> data_format() {

            if constexpr (sizeof(T) == 1)
                return ZString<4>("%c");

            else if constexpr (std::is_floating_point_v<T>) {
                constexpr ZString<2> prefix(
                    std::is_same_v<T, float>  ? ""  :
                    std::is_same_v<T, double> ? "l" : "ll"
                );
                return ZString<1>("%") + prefix + ZString<1>("f");
            }
            
            else {
                constexpr ZString<2> prefix(
                    sizeof(T) <= sizeof(int)            ? "" :
                    std::is_same_v<long, T>
                    || std::is_same_v<unsigned long, T> ? "l" : "ll"
                );
                constexpr ZString<1> suffix(std::is_unsigned_v<T> ? "u" : "d");
                return (ZString<1>("%") + prefix) + suffix;
            }
        }

        // Returns the format string for printf/scanf for the given types
        template<Arithm... T>
        static constexpr auto build_format_string() {
            return (data_format<T>() + ...);
        }

        template<Arithm... T>
        static constexpr auto build_format_string_space() {
            return ((data_format<T>() + ZString<1>(" ")) + ...);
        }

        // Helper function to scan
        template<Arithm T, std::size_t count, std::size_t size>
        exit_status scanf_array_(const ZString<size>& format, std::array<T, count>& t) const {
            auto& [ ...t_i ] = t;
            return scanf(format, t_i...);
        }

        template<typename... Args, std::size_t size>
        int scanf_count(const ZString<size>& format, Args&... x) const {
            return fscanf(f, format.c_str(), &x...);
        }

        template<typename... Args, std::size_t size>
        exit_status scanf(const ZString<size>& format, Args&... x) const {
            const int ret = scanf_count(format, x...);
            return exit_status_of(ret == sizeof...(Args));
        }

        template<typename... Args, std::size_t size>
        exit_status scanf_rewind_if_failure(const ZString<size>& format, Args&... x) const {
            
            skip_whitespace();
            const std::size_t pos = position();

            const int ret = scanf_count(format, x...);
            
            if (ret == sizeof...(Args))
                return exit_status::Success;

            rewind(pos);
            return exit_status::Failure;
        }


    public:
        enum class error {
            FileNotFound, FileCouldNotBeCreated,
            WrongParameter,
            ScanError
        };

        file(const std::string& filename, const std::string& mode_s = "r") :
            f(fopen(filename.c_str(), mode_s.c_str())), mode_(string_of_mode(mode_s)) {

            if (f == nullptr) {
                using enum mode;
                using enum error;
                const bool read = mode_ == R || mode_ == RB;
                const std::string message = read ? "not found" : "could not be created";
                std::printf("\rFile %s %s\n", filename.c_str(), message.c_str());
                throw read ? FileNotFound : FileCouldNotBeCreated;
            }
        }

        file(file&&)                 = delete;
        file& operator=(file&&)      = delete;
        file(const file&)            = delete;
        file& operator=(const file&) = delete;

        void close() noexcept {
            if (f != nullptr)
                fclose(f);
            f = nullptr;
        }

        ~file() noexcept {
            close();
        }

        [[nodiscard]] std::size_t position() const {
            return ftell(f);
        }

        void seek(std::size_t pos) const {
            fseek(f, pos, SEEK_SET);
        }

        void rewind(std::size_t pos = 0) const {
            if (pos > position()) {
                std::printf("file::rewind: rewind to position set ahead of current position");
                throw file::error::WrongParameter;
            }
            seek(pos);
        }

        [[nodiscard]] std::size_t length() const {
            const std::size_t pos = position();
            fseek(f, 0, SEEK_END); // go to end of file
            const std::size_t length = position();
            seek(pos);
            return length;
        }

        [[nodiscard]] bool eof() const {
            return feof(f);
        }

        template<class T>
        exit_status read(std::span<T> buffer) const {
            const std::size_t ret = fread(static_cast<void*>(buffer.data()), sizeof(T), buffer.size(), f);
            return exit_status_of(ret == buffer.size());
        }

        template<class T, std::size_t extent>
        requires (extent != std::dynamic_extent)
        exit_status read(std::span<T, extent> buffer) const {
            const std::size_t ret = fread(static_cast<void*>(buffer.data()), sizeof(T), extent, f);
            return exit_status_of(ret == extent);
        }

        template<class T>
        exit_status read(std::vector<T>& buffer) const {
            return read(std::span(buffer));
        }

        enum class string_reading_type {
            ReadAll, StopAtSpace, RemoveTrailingCRLF
        };

        // Returns a string of length at most max_length (plus the '\0' terminating-character)
        [[nodiscard]] std::string read_string(unsigned int max_length = MAX_STRING_LENGTH, string_reading_type type = string_reading_type::StopAtSpace) const {
            if (max_length > MAX_STRING_LENGTH) {
                std::printf("Error: read_string can read a string of length at most %d\n", MAX_STRING_LENGTH);
                throw file::error::WrongParameter;
            }
            constexpr std::size_t LENGTH = MAX_STRING_LENGTH + 1;
            std::array<char, LENGTH> t = make_array<char, LENGTH>('\0');

            skip_whitespace();

            const std::size_t pos = position();

            std::ignore = fgets(t.data(), max_length + 1, f);
            std::string out(t.data());

            // Postprocessing
            using enum string_reading_type;
            switch (type) {
                case StopAtSpace: {
                    const std::size_t n = std::min(std::min(
                        out.find_first_of(' '),
                        out.find_first_of('\r')),
                        out.find_first_of('\n')
                    );
                    const bool found = (n != std::string::npos);
                    if (found) {
                        out.resize(n);
                        seek(pos + n + 1);
                    }
                    break;
                }
                case RemoveTrailingCRLF: {
                    if (out.ends_with("\r\n")) return out.substr(0, out.length() - 2);
                    if (out.ends_with("\n"))   return out.substr(0, out.length() - 1);
                    break;
                }
                default:
                    break;
            }

            return out;
        }

        // Returns the next line as a string
        [[nodiscard]] std::string read_line() const {
            
            constexpr std::size_t LENGTH = MAX_STRING_LENGTH + 1;
            std::array<char, LENGTH> t = make_array<char, LENGTH>('\0');

            skip_whitespace();

            const std::size_t pos = position();

            fgets(t.data(), LENGTH, f);
            std::string out(t.data());
                
            const std::size_t n = std::min(out.find_first_of('\r'), out.find_first_of('\n'));
            if (n != std::string::npos) {
                out.resize(n);
                seek(pos + n + 1);
            }
            
            return out;
        }

        int getc() const {
            return fgetc(f);
        }

        int ungetc(int c) const {
            return std::ungetc(c, f);
        }

        int peek_next() const {
            const char c = getc();
            ungetc(c);
            return c;
        }

        exit_status skip(unsigned int n) const {
            std::vector<char> buffer(n);
            return read<char>(buffer);
        }

        void skip_char(char ch, unsigned int count = INFINITE) const {
            if (eof()) return;
            char c = '\0';
            while (!eof() && (c = getc()) == ch && ((count--) != 0));
            ungetc(c);
        }

        void skip_until_char(char ch, unsigned int count = INFINITE) const {
            if (eof()) return;
            char c = '\0';
            while (!eof() && (c = getc()) != ch && ((count--) != 0));
            ungetc(c);
        }

        void skip_whitespace() const {
            char c;
            while (!eof()) {
                c = getc();
                if (!(c == ' ' || c == '\r' || c == '\n' || c == '\t')) {
                    ungetc(c);
                    break;
                }
            }
        }
        
        void skip_line() const {
            skip_until_char('\n');
            skip_char('\n');
        }


        template<typename... Args>
        int scanf_count(const std::string& format_s, Args&... x) const {
            return scanf_count(ZString<MAX_FORMAT_SIZE>(format_s), x...);
        }

        exit_status scanf(const std::string& string) const {

            // Skip whitespace in string
            int i = 0;
            while (string[i] == ' ')
                i++;
            const std::string_view string_to_match = std::string_view(string).substr(i);

            const std::size_t pos = position();
            const std::string read = read_string(string_to_match.length(), string_reading_type::ReadAll);
            
            if (read.starts_with(string.substr(i)))
                return exit_status::Success;
            
            // Rewind to last character successfully matched
            std::size_t index = 0;
            while (read[index] == string[index])
                index++;
            seek(pos + index);
            return exit_status::Failure;
        }

        template<typename... Args>
        exit_status scanf(const std::string& format_s, Args&... x) const {
            return scanf(ZString<MAX_FORMAT_SIZE>(format_s), x...);
        }

        template<typename... Args>
        exit_status scanf_rewind_if_failure(const std::string& format_s, Args&... x) const {
            return scanf_rewind_if_failure(ZString<MAX_FORMAT_SIZE>(format_s), x...);
        }

        exit_status scanf_rewind_if_failure(const std::string& string) const {
            const std::size_t pos = position();
            
            const exit_status status = scanf(string);
            if (status == exit_status::Failure)
                rewind(pos);

            return status;
        }


        template<Arithm... T>
        requires (sizeof...(T) > 1)
        std::optional<std::tuple<T...>> scan() const {
            std::tuple<T...> t;
            constexpr auto format = build_format_string<T...>();
            const exit_status status = scanf(format, std::get<T>(t)...);
            return optional_of(status, std::move(t));
        }

        template<Arithm T>
        std::optional<T> scan() const {
            constexpr auto format = build_format_string<T>();
            T x;
            const exit_status status = scanf(format, x);
            return optional_of<T>(status, std::move(x));
        }

        template<Arithm T, std::size_t count>
        std::array<T, count> scan() const {

            constexpr auto format = concat_count<count, 4>(data_format<T>());

            std::array<T, count> t;
            const exit_status status = scanf_array_(format, t);
            throw_if_failure(status, file::error::ScanError);
            return t;
        }

        template<typename T>
        std::optional<T> scan() const {
            T x;
            const exit_status status = read<T>({ &x, 1 });
            return optional_of<T>(status, std::move(x));
        }

        [[nodiscard]] std::vector<unsigned char> extract_from() const {
            const std::size_t pos = position();
            const std::size_t len = length() - pos;
            std::vector<unsigned char> content(len + 1);
            read<unsigned char>(content);
            content[len] = '\0';
            return content;
        }

        [[nodiscard]] std::vector<unsigned char> extract() const {
            rewind();
            return extract_from();
        }

        void cat() const {
            const std::vector<unsigned char> content = extract();
            std::printf("%s", content.data());
        }

        void cat_from() const {
            const std::vector<unsigned char> content = extract_from();
            std::printf("%s", content.data());
        }

        template<class T>
        exit_status write(std::span<const T> buffer) const {
            const std::size_t ret = fwrite(static_cast<const void*>(buffer.data()), sizeof(T), buffer.size(), f);
            return exit_status_of(ret == buffer.size());
        }

        template<class T, std::size_t extent>
        exit_status write(std::span<const T, extent> buffer) const {
            const std::size_t ret = fwrite(static_cast<const void*>(buffer.data()), sizeof(T), extent, f);
            return exit_status_of(ret == extent);
        }

        template<class T>
        exit_status write(const std::vector<T>& buffer) const {
            return write(std::span(buffer));
        }

        template<typename T, T value, std::size_t count>
        exit_status write() const {
            constexpr std::array<T, count> t = make_array<T, count>(value);
            return write<T, count>(t);
        }

        template<typename T, T value>
        exit_status write(int count) const {
            const std::vector<T> v(count, value);
            return write(v);
        }

        exit_status write(const std::string& s) const {
            return exit_status_of(fprintf(f, "%s", s.c_str()));
        }

        template<typename T, typename... Ti>
        requires (std::is_same_v<T, Ti> && ...)
        exit_status write(const Ti... x) const {
            constexpr std::size_t extent = sizeof...(Ti);
            const std::array<T, extent> t = { x... };
            return write(std::span<const T, extent>(t));
        }

        template<typename... Args>
        exit_status printf(const std::string& format, const Args... args) const {
            const int ret = fprintf(f, format.c_str(), args...);
            return exit_status_of(ret != EOF);
        }

        exit_status printf(const std::string& s) const {
            return printf("%s", s.c_str());
        }

        void display_current() const {
            const std::size_t pos = position();
            const std::string line = read_line();
            std::printf("%s\n", line.c_str());
            rewind(pos);
        }
};