#include <iostream>
#include <cstdlib>

#include <array>
#include <vector>
#include <bit>
#include <memory>
#include <stack>

#include <variant>
#include <expected>

#include <cstdio>
#include <string>
#include <sstream>

#include "light/vector.hpp"
#include "auxiliary/utils.hpp"

[[maybe_unused]]
static void compare_sizes() {
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

[[maybe_unused]]
static void test_variant() {
    using T = std::variant<int, float, short>;
    T a;

    std::cout << sizeof(T) << std::endl;
    auto x = a.index();
    std::cout << sizeof(x) << std::endl;
}

[[maybe_unused]]
static void test_expected() {
    using U = std::pair<int, short>;
    using T = std::expected<U, error_type>;

    std::cout << "sizeof(U) = " << sizeof(U) << std::endl;
    std::cout << "sizeof(error_type) = " << sizeof(error_type) << std::endl;
    std::cout << "sizeof(std::expected<U, error_type>) = " << sizeof(T) << std::endl;
    std::cout << "sizeof(std::optional<U>) = " << sizeof(std::optional<U>) << std::endl;
}

[[maybe_unused]]
static void test_unique_ptr() {
    std::unique_ptr<Box> pt(new Box(rt::vector(), rt::vector(), rt::vector(), 1, 1, 1));
    std::cout << "sizeof(std::unique_ptr) = " << sizeof(pt) << std::endl;
    std::cout << "sizeof(Box*) = " << sizeof(Box*) << std::endl;

    auto pt2 = std::make_unique<Box>(rt::vector(), rt::vector(), rt::vector(), 1, 1, 1);
    std::cout << "sizeof(std::unique_ptr) = " << sizeof(pt2) << std::endl;
}

[[maybe_unused]]
static void test_sizeof_vector() {
    std::cout << "sizeof(vector) = " << sizeof(std::vector<void*>) << std::endl;
    struct array {
        std::unique_ptr<int> data;
        std::size_t size;
    };
    std::cout << "sizeof(custom array) = " << sizeof(array) << std::endl;
}

template<typename X, std::size_t size = 10>
class C {

    public:
        X data[size];

        C() { std::cout << "size = " << size << std::endl; }
};

[[maybe_unused]]
static void test_template() {
    C<int> c1;
    C<int, 2> c2;
}

[[maybe_unused]]
static void test_stack() {
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
}

[[maybe_unused]]
static void test_parsing() {
    std::string line =
        // "1/2/3 4/5/6 7/8/9";
        "1//3 4//6 7//9";
        //"1 4 7";

    // int v1, v2, v3, v4, v5;
    // int vt1, vt2, vt3, vt4, vt5;
    // int vn1, vn2, vn3, vn4, vn5;

    /*
        const int ret = sscanf(line.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d",
            &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);

        std::cout << "ret = " << ret << std::endl;
        std::cout << "vi: ";
        for (int v : { v1, v2, v3 })
            std::cout << v << " ";
        std::cout << "vti: ";
        for (int vt : { vt1, vt2, vt3 })
            std::cout << vt << " ";
        std::cout << "vni: ";
        for (int vn : { vn1, vn2, vn3 })
            std::cout << vn << " ";
    */

    std::istringstream stream(line);
    /*
    // stream >> v1 >> v2 >> v3 >> v4 >> v5;
    int v[5];
    int i = 0;
    while (stream >> v[i])
        i++;

    // std::cout << v1 << " " << v2 << " " << v3 << " " << v4 << " " << v5 << std::endl;
    for (int j = 0; j < i; j++)
        std::cout << v[j] << " ";
    std::cout << std::endl;
    */

    int v[5];
    //int vt[5];
    int vn[5];
    int i = 0;
    char d1, d2;
    while (i < 5 && stream >> v[i] >> d1 >> d2 >> vn[i])
        i++;

    for (int j = 0; j < i; j++) {
        std::cout << v[j] << d1 << d2 << vn[j] << " ";
    }
    std::cout << std::endl;
}

int main() {

    test_parsing();

    return EXIT_SUCCESS;
}