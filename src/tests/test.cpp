#include <iostream>
#include <cstdlib>

#include <array>
#include <vector>
#include <bit>
#include <memory>

#include <variant>
#include <expected>

#include "light/vector.hpp"
#include "auxiliary/utils.hpp"

using real = double;

void compare_sizes() {
    std::vector<real> v(8);
    std::cout << "sizeof(array<real, 8>) = " << sizeof(std::array<real, 8>) << std::endl;
    std::cout << "sizeof(v) = " << sizeof(v) << std::endl;
    std::cout << "total size v = " << sizeof(v) + v.capacity() * sizeof(real) << std::endl;
}

enum class error_type {
    ParsingError, FileNotFound
};

class Box {
    rt::vector center, n1, n2, n3;
    [[maybe_unused]] real l1, l2, l3;

    public:
        Box(const rt::vector& center, const rt::vector& x_axis, const rt::vector& y_axis, real l1_, real l2_, real l3_)
            : center(center), n1(x_axis.unit()), n2(y_axis.unit()), n3(x_axis ^ y_axis), l1(l1_), l2(l2_), l3(l3_) {} 
};

template<typename X, std::size_t size = 10>
class C {

    public:
        X data[size];

        C() { std::cout << "size = " << size << std::endl; }
};

#include <stack>

int main() {

    // using T = std::variant<int, float, short>;
    // T a;

    // std::cout << sizeof(T) << std::endl;
    // auto x = a.index();
    // std::cout << sizeof(x) << std::endl;

    // using U = std::pair<int, short>;
    // using T = std::expected<U, error_type>;

    // std::cout << "sizeof(U) = " << sizeof(U) << std::endl;
    // std::cout << "sizeof(error_type) = " << sizeof(error_type) << std::endl;
    // std::cout << "sizeof(std::expected<U, error_type>) = " << sizeof(T) << std::endl;
    // std::cout << "sizeof(std::optional<U>) = " << sizeof(std::optional<U>) << std::endl;

    std::unique_ptr<Box> pt(new Box(rt::vector(), rt::vector(), rt::vector(), 1, 1, 1));
    std::cout << "sizeof(std::unique_ptr) = " << sizeof(pt) << std::endl;
    std::cout << "sizeof(Box*) = " << sizeof(Box*) << std::endl;

    auto pt2 = std::make_unique<Box>(rt::vector(), rt::vector(), rt::vector(), 1, 1, 1);
    std::cout << "sizeof(std::unique_ptr) = " << sizeof(pt2) << std::endl;


    // std::cout << "sizeof(vector) = " << sizeof(std::vector<void*>) << std::endl;
    // struct array {
    //     std::unique_ptr<int> data;
    //     std::size_t size;
    // };
    // std::cout << "sizeof(custom array) = " << sizeof(array) << std::endl;

    C<int> c1;
    C<int, 2> c2;

    int values[] = { 1, 2, 3, 4, 5 };
    std::stack<int> st;
    
    // for (int v : { 1, 2, 3, 4, 5 })
    //     st.push(v);
    st.push_range(values);
    
    while (not st.empty()) {
        int v = st.top();
        std::cout << v << " ";
        st.pop();
    }
    std::cout << std::endl;

    real x = 0_r;
    real invx = 1.0_r / x;

    printf("1 / 0 = %lf\n", invx);

    return EXIT_SUCCESS;
}