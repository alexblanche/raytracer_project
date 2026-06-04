#pragma once

#include "auxiliary/exit_status.hpp"

#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>
#include <array>

#include <optional>

// Used to ignore unused indices from std::index_sequence
template<typename T>
static consteval T id(size_t, T value) {
    return value;
}

template<typename T, size_t... i>
static consteval inline std::array<T, sizeof...(i)> make_array_aux_(T value, std::index_sequence<i...>) {
    return { id(i, value)... };
}

// Returns an array with all the elements initialized to value
template<typename T, size_t count>
static consteval inline std::array<T, count> make_array(T value) {
    return make_array_aux_<T>(value, std::make_index_sequence<count>());
}

template<size_t count, size_t... i>
static consteval inline std::string string_concat_aux_(const std::string& value, std::index_sequence<i...>) {
    return (id(i, value) + ...);
}

// Returns a string made up of the string value repeated count times
template<size_t count>
static consteval inline std::string string_concat(const std::string& value) {
    return string_concat_aux_<count>(value, std::make_index_sequence<count>());
}

template<typename T, typename U>
concept Convertible = std::is_convertible_v<U, T>;

template<typename T>
concept Arithm = std::is_arithmetic_v<T>;

template<Arithm T>
static consteval std::string data_format() {
    constexpr std::string prefix = (sizeof(T) <= sizeof(int)) ? "" : (sizeof(T) == sizeof(long int)) ? "l" : "ll";
    constexpr std::string suffix =
        std::is_floating_point_v<T> ? "f" :
        sizeof(T) == 1              ? "c" :
        std::is_unsigned_v<T>       ? "u" : "d";
    return "%" + prefix + suffix;
}

template<Arithm... T>
static consteval std::string build_format_string() {
    return (data_format<T>() + ...);
}

constexpr unsigned int MAX_STRING_LENGTH = 1000;

class file {
    
    enum class mode {
        R, RB, W, WB, A, AB
    };
    static constexpr mode string(const std::string& s) {
        using enum mode;
        if (s == "r" ) return R;
        if (s == "rb") return RB;
        if (s == "w" ) return W;
        if (s == "wb") return WB;
        if (s == "a" ) return A;
        if (s == "ab") return AB;
        return R; // default
    }

    private:
        FILE *f;
        mode mode;

    private:
        // Helper function to scan
        template<Arithm T, size_t count, size_t... i>
        exit_status scanf_array_(const std::string& format, std::array<T, count>& t, std::index_sequence<i...>) const {
            return scanf(format, t[i]...);
        }


    public:
        file(const std::string& filename, const std::string& mode_s = "r") :
            f(fopen(filename.c_str(), mode_s.c_str())), mode(string(mode_s)) {

            if (f == nullptr) {
                const std::string message = (mode == mode::R || mode == mode::RB) ? "not found" : "could not be created";
                printf("File %s %s\n", filename.c_str(), message.c_str());
                throw std::runtime_error("");
            }
        }

        void close() noexcept {
            if (f != nullptr)
                fclose(f);
            f = nullptr;
        }

        ~file() noexcept {
            close();
        }

        [[nodiscard]] size_t position() const {
            return ftell(f);
        }

        void seek(size_t pos) const {
            fseek(f, pos, SEEK_SET);
        }

        void rewind() const {
            seek(0);
        }

        [[nodiscard]] size_t length() const {
            const size_t pos = position();
            fseek(f, 0, SEEK_END); // go to end of file
            const size_t length = position();
            seek(pos);
            return length;
        }

        [[nodiscard]] bool eof() const {
            return feof(f);
        }

        template<class T>
        exit_status read(std::span<T> buffer) const {
            const int ret = fread(static_cast<void*>(buffer.data()), sizeof(T), buffer.size(), f);
            return exit_status_of(ret == static_cast<int>(buffer.size()));
        }

        template<class T, size_t extent>
        requires (extent != std::dynamic_extent)
        exit_status read(std::span<T, extent> buffer) const {
            const int ret = fread(static_cast<void*>(buffer.data()), sizeof(T), extent, f);
            return exit_status_of(ret == extent);
        }

        template<class T>
        exit_status read(std::vector<T>& buffer) const {
            return read<T>(std::span<T>(buffer));
        }


        // Returns a string of length at most max_length (plus the '\0' terminating-character)
        [[nodiscard]] std::string read_string(unsigned int max_length = MAX_STRING_LENGTH) const {
            if (max_length > MAX_STRING_LENGTH) {
                std::printf("Error: read_string can read a string of length at most %d\n", MAX_STRING_LENGTH);
                throw std::runtime_error("");
            }
            constexpr size_t LENGTH = MAX_STRING_LENGTH + 1;
            std::array<char, LENGTH> t = make_array<char, LENGTH>('\0');
            fgets(t.data(), max_length + 1, f);
            return std::string(t.data());
        }

        int getc() const {
            return fgetc(f);
        }

        int ungetc(int c) const {
            return std::ungetc(c, f);
        }

        exit_status skip(unsigned int n) const {
            std::vector<char> buffer(n);
            return read<char>(buffer);
        }

        void skip_line() const {
            char c;
            while (!eof() && (c = getc()) != '\n');
            ungetc(c);
        }

        template<typename... Args>
        int scanf_count(const std::string& format, Args&... x) const {
            return fscanf(f, format.c_str(), &x...);
        }

        template<typename... Args>
        exit_status scanf(const std::string& format, Args&... x) const {
            const int ret = scanf_count(format, x...);
            return exit_status_of(ret == sizeof...(Args));
        }


        template<Arithm... T>
        requires (sizeof...(T) > 1)
        std::optional<std::tuple<T...>> scan() const {
            std::tuple<T...> t;
            constexpr std::string format = build_format_string<T...>();
            const exit_status status = scanf(format, std::get<T>(t)...);
            return optional_of(status, std::move(t));
        }

        template<Arithm T>
        std::optional<T> scan() const {
            constexpr std::string format = build_format_string<T>();
            T x;
            const exit_status status = scanf(format, x);
            return optional_of<T>(status, std::move(x));
        }

        template<Arithm T, size_t count>
        std::array<T, count> scan() const {
            constexpr std::string df = data_format<T>();
            constexpr std::string format = string_concat<count>(df);
            std::array<T, count> t;
            const exit_status status = scanf_array_(format, t, std::make_index_sequence<count>());
            if (status == exit_status::Failure)
                throw std::runtime_error("scan<T, count>");
            return t;
        }

        template<typename T>
        std::optional<T> scan() const {
            T x;
            const exit_status status = read<T>({ &x, 1 });
            return optional_of<T>(status, std::move(x));
        }

        [[nodiscard]] std::vector<unsigned char> extract_from() const {
            const size_t pos = position();
            const size_t len = length() - pos;
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
            const int ret = fwrite(static_cast<const void*>(buffer.data()), sizeof(T), buffer.size(), f);
            return exit_status_of(ret == buffer.size());
        }

        template<class T, size_t extent>
        exit_status write(std::span<const T, extent> buffer) const {
            const int ret = fwrite(static_cast<const void*>(buffer.data()), sizeof(T), extent, f);
            return exit_status_of(ret == extent);
        }

        template<class T>
        exit_status write(const std::vector<T>& buffer) const {
            return write(buffer);
        }

        template<typename T, T value, size_t count>
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

        template<typename T, Convertible<T>... Ti>
        exit_status write(const Ti... x) const {
            constexpr size_t extent = sizeof...(Ti);
            const std::array<T, extent> t = { static_cast<T>(x)... };
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
};