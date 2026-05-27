#pragma once

#include "auxiliary/exit_status.hpp"

#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

#include <optional>

class file {
    
    private:
        FILE *f;

    public:
        file(const std::string& filename, const std::string& mode = "r") :
            f(fopen(filename.c_str(), mode.c_str())) {

            if (f == nullptr) {
                printf("File %s not found\n", filename.c_str());
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

        int getc() const {
            return fgetc(f);
        }

        exit_status skip(int n) const {
            std::vector<char> buffer(n);
            return read<char>({ buffer.data(), buffer.data() + n });
        }

        void skip_line() const {
            while (!eof() && getc() != '\n');
        }

        template<typename... Args>
        exit_status scanf(const std::string& format, Args&... pt) const {
            const int ret = fscanf(f, format.c_str(), &pt...);
            return exit_status_of(ret == sizeof...(Args));
        }

        template<class T>
        std::optional<T> scan() const {
            T x;
            const exit_status status = read<T>({ &x, 1 });
            return optional_of<T>(status, std::move(x));
        }

        template<typename T>
        requires std::is_integral_v<T>
        std::optional<T> scan() const {
            T x;
            constexpr std::string format = (std::is_signed_v<T>) ? "%lld" : "%llu";
            const exit_status status = scanf<T>(format, x);
            return optional_of<T>(status, std::move(x));
        }

        template<typename T>
        requires std::is_floating_point_v<T>
        std::optional<T> scan() const {
            T x;
            constexpr std::string format = (sizeof(T) == sizeof(float)) ? "%f" : "%lf";
            const exit_status status = scanf<T>(format, x);
            return optional_of<T>(status, std::move(x));
        }

        std::string extract_from() const {
            const size_t pos = position();
            const size_t len = length() - pos;
            std::string s(len + 1, '\0');
            const exit_status status = read<char>(s);
            printf("status = %s\n", status == exit_status::Success ? "success" : "failure");
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
        exit_status write(std::span<T> buffer) const {
            const int ret = fwrite(static_cast<void*>(buffer.data()), sizeof(T), buffer.size(), f);
            return exit_status_of(ret == buffer.size());
        }

        template<class T, size_t extent>
        exit_status write(std::span<T, extent> buffer) const {
            const int ret = fwrite(static_cast<void*>(buffer.data()), sizeof(T), extent, f);
            return exit_status_of(ret == extent);
        }

        exit_status write(const std::string& s) const {
            return exit_status_of(fprintf(f, "%s", s.c_str()));
        }

        template<typename... Args>
        exit_status printf(const std::string& format, Args... args) const {
            const int ret = fprintf(f, format.c_str(), args...);
            return exit_status_of(ret != EOF);
        }
};