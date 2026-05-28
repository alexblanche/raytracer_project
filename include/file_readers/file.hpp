#pragma once

#include "auxiliary/exit_status.hpp"

#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>
#include <array>

#include <optional>

template<typename T, typename U>
concept Convertible = std::is_convertible_v<U, T>;

template<typename T>
concept Arithm = std::is_arithmetic_v<T>;

template<Arithm T>
constexpr std::string data_format() {
    constexpr std::string prefix = (sizeof(T) <= sizeof(int)) ? "" : (sizeof(T) == sizeof(long int)) ? "l" : "ll";
    constexpr std::string suffix =
        std::is_floating_point_v<T> ? "f" :
        sizeof(T) == 1              ? "c" :
        std::is_unsigned_v<T>       ? "u" : "d";
    return "%" + prefix + suffix + " ";
}

template<Arithm... T>
static constexpr std::string build_format_string() {
    return (data_format<T>() + ...);
}

constexpr int MAX_STRING_LENGTH = 1000;

class file {
    
    enum class mode {
        R, RB, W, WB, A, AB
    };
    static constexpr mode string(const std::string& s) {
        if (s == "r" ) return mode::R;
        if (s == "rb") return mode::RB;
        if (s == "w" ) return mode::W;
        if (s == "wb") return mode::WB;
        if (s == "a" ) return mode::A;
        if (s == "ab") return mode::AB;
        return mode::R;
    }

    private:
        FILE *f;
        mode mode;

    public:
        file(const std::string& filename, const std::string& mode_s = "r") :
            f(fopen(filename.c_str(), mode_s.c_str())), mode(string(mode_s)) {

            if (f == nullptr) {
                const std::string message = (mode == mode::R || mode == mode::RB) ? "not found" : "could not be created";
                printf("File %s %s\n", filename.c_str(), message.c_str());
                throw std::runtime_error("");
            }
        }

        void close() {
            fclose(f);
            f = nullptr;
        }

        ~file() {
            close();
        }

        size_t length() const {
            const size_t position = ftell(f);
            fseek(f, 0, SEEK_END);
            const size_t length = ftell(f);
            fseek(f, position, SEEK_SET);
            return length;
        }

        bool eof() const {
            return feof(f);
        }

        size_t position() const {
            return ftell(f);
        }

        void seek(size_t pos) const {
            fseek(f, pos, SEEK_SET);
        }

        void rewind() const {
            seek(0);
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

        std::string read_string(int max_length = MAX_STRING_LENGTH) const {
            std::array<char, MAX_STRING_LENGTH> t;
            t.fill(0);
            fgets(t.data(), max_length, f);
            return std::string(t.data());
        }

        // Experimental
        // template<class T>
        // exit_status read(T* elt) const {
        //     return read({ *elt });
        // }

        int getc() const {
            return fgetc(f);
        }

        int ungetc(int c) const {
            return std::ungetc(c, f);
        }

        exit_status skip(int n) const {
            std::vector<char> buffer(n);
            return read<char>(buffer);
        }

        void skip_line() const {
            char c;
            while (!eof() && (c = getc()) != '\n');
            ungetc(c);
        }

        template<typename... Args>
        exit_status scanf(const std::string& format, Args&... x) const {
            const int ret = fscanf(f, format.c_str(), &x...);
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

        template<Arithm T, size_t count, size_t... i>
        exit_status scanf_array_(const std::string& format, std::array<T, count>& t, std::index_sequence<i...>) const {
            return scanf(format, t[i]...);
        }

        template<Arithm T, size_t count>
        std::array<T, count> scan() const {
            constexpr std::string df = data_format<T>();
            std::string format = "";
            for (size_t i = 0; i < count; i++)
                format += df;
            std::array<T, count> t;
            scanf_array_(format, t, std::make_index_sequence<count>());
            return t;
        }

        template<typename T>
        std::optional<T> scan() const {
            T x;
            const exit_status status = read<T>({ &x, 1 });
            return optional_of<T>(status, std::move(x));
        }

        std::string extract_from() const {
            const size_t pos = position();
            const size_t len = length() - pos;
            std::string s(len + 1, '\0');
            read<char>(s);
            return s;
        }

        std::string extract() const {
            rewind();
            return extract_from();
        }

        void cat() const {
            const std::string content = extract();
            std::printf("%s", content.c_str());
        }

        void cat_from() const {
            const std::string content = extract_from();
            std::printf("%s", content.c_str());
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
            std::array<T, count> t;
            t.fill(value);
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

        template<typename T, Convertible<T>... Args>
        exit_status write(const Args... x) const {
            constexpr size_t extent = sizeof...(Args);
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