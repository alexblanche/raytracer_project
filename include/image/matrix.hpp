#pragma once

#include "screen/color.hpp"

#include <span>
#include <vector>
#include <iterator>

// Replaces the one from bmp_reader.hpp
struct dimensions {
    std::size_t width, height;
};

class matrix {

    public:
        using row       = std::span<rt::color>;
        using const_row = std::span<const rt::color>;

        std::vector<rt::color> data;
        std::size_t width;
        std::size_t height;

        matrix() {}

        matrix(int width, int height)
            : data(height * width),
              width(width), height(height) {}

        matrix(matrix&&)                 noexcept = default;
        matrix& operator=(matrix&&)      noexcept = default;
        matrix(const matrix&)            = delete;
        matrix& operator=(const matrix&) = delete;

        dimensions get_dimensions() const {
            return { width, height };
        }

        matrix::row get_row(int j) {
            return { &data[j * width], width };
        }

        matrix::const_row get_row(int j) const {
            return { &data[j * width], width };
        }

        matrix::row operator[](int j) {
            return get_row(j);
        }

        matrix::const_row operator[](int j) const {
            return get_row(j);
        }

        const rt::color& get(int row, int col) const {
            return data[row * width + col];
        }

        rt::color& get(int row, int col) {
            return data[row * width + col];
        }

        const rt::color& operator[](int row, int col) const {
            return get(row, col);
        }

        rt::color& operator[](int row, int col) {
            return get(row, col);
        }

        void apply_gamma(real gamma);


    // Random-access iterator
    struct iterator {
        private:
            std::size_t j;              // row index
            matrix* mat;

        public:
            using difference_type = int;
            using value_type = row;

            iterator(const int j, matrix* p_matrix)
                : j(j), mat(p_matrix) {}

            iterator()
                : j(0), mat(nullptr) {}

            iterator(iterator&&)                 noexcept = default;
            iterator(const iterator&)            noexcept = default;
            iterator& operator=(iterator&&)      noexcept = default;
            iterator& operator=(const iterator&) noexcept = default;

            ~iterator() = default;

            value_type operator*() const {
                return (*mat)[j];
            }

            bool operator==(std::default_sentinel_t) const {
                return j == mat->height;
            }

            bool operator==(const iterator& other) const {
                return j == other.j;
            }

            iterator& operator++() {
                j++;
                return *this;
            }

            iterator operator++(int) {
                auto prev = *this;
                ++(*this);
                return prev;
            }

            iterator& operator--() {
                j--;
                return *this;
            }

            iterator operator--(int) {
                auto prev = *this;
                --(*this);
                return prev;
            }

            iterator operator+(difference_type n) const {
                return iterator(j + n, mat);
            }

            iterator operator-(difference_type n) const {
                return iterator(j - n, mat);
            }

            iterator& operator+=(difference_type n) {
                j += n;
                return *this;
            }

            iterator& operator-=(difference_type n) {
                j -= n;
                return *this;
            }

            difference_type operator-(const iterator& other) const {
                return j - other.j;
            }

            value_type operator[](difference_type n) const {
                return (*mat)[j + n];
            }

            bool operator<(const iterator& other) const {
                return j < other.j;
            }

            bool operator<=(const iterator& other) const {
                return j <= other.j;
            }

            bool operator>(const iterator& other) const {
                return j > other.j;
            }

            bool operator>=(const iterator& other) const {
                return j >= other.j;
            }
    };

    iterator begin() {
        return iterator(0, &(*this));
    }

    iterator end() {
        return iterator(height, &(*this));
    }

    
    class input_iterator {
        private:
            std::size_t j;
            inline static const matrix* mat;

        public:
            using difference_type = int;
            using value_type = const_row;

            input_iterator(int j = 0, const matrix* p_mat = nullptr)
                : j(j) { mat = p_mat; }

            input_iterator(input_iterator&&)                 noexcept = default;
            input_iterator& operator=(input_iterator&& it)   noexcept = default;
            input_iterator(const input_iterator&)            = delete;
            input_iterator& operator=(const input_iterator&) = delete;

            ~input_iterator() = default;
            
            value_type operator*() const {
                return (*mat)[j];
            }

            input_iterator& operator++() {
                j++;
                return *this;
            }

            void operator++(int) {
                ++(*this);
            }

            bool operator==(std::default_sentinel_t) const {
                return j == mat->height;
            }

            bool operator==(const input_iterator& other) const {
                return j == other.j;
            }
    };

    input_iterator begin() const {
        return input_iterator(0, &(*this));
    }

    input_iterator end() const {
        return input_iterator(height, &(*this));
    }
};

inline matrix::iterator operator+(matrix::iterator::difference_type n, const matrix::iterator& it) {
    return it + n;
}

static_assert(std::random_access_iterator<matrix::iterator>);
static_assert(std::input_iterator<matrix::input_iterator>);
