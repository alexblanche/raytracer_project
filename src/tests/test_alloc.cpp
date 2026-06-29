#include <iostream>
#include <cstdlib>

#include <vector>
#include <array>
#include <span>

#include <bit>
#include <memory>

#include "auxiliary/custom_stack.hpp"
#include "auxiliary/utils.hpp"

using real = double;

class A {
    public:
        int id;
        static inline int cpt = 0;

        A(int x)      : id(cpt++) { std::cout << "A(int) (x = " << x << ") (" << id << ")" << std::endl; }

        A()           : id(cpt++) { std::cout << "A()          (" << id << ")" << std::endl; }
        A(A&& a)      : id(cpt++) { std::cout << "A(A&&)       (" << id << ", from " << a.id << ")" << std::endl; }
        A(const A& a) = default;//: id(cpt++) { std::cout << "A(const A&)  (" << id << ", from " << a.id << ")" << std::endl; }
        A& operator=(A&& a)       { id = cpt++;
                                    std::cout << "A = A&&      (" << id << ", from " << a.id << ")" << std::endl; return *this; }
        A& operator=(const A& a) { id = cpt++;
                                std::cout << "A = const A& (" << id << ", from " << a.id << ")" << std::endl; return *this; }
        ~A() = default;//                     { std::cout << "~A()         (" << id << ")" << std::endl; }
};

class B {
    public:
        static constexpr unsigned int N = 3;
        //using Array = std::array<A, N>;
        //Array arr;
        A arr[N];

        B() : arr{} { std::cout << "B()" << std::endl; }
        //B(Array&& a) : arr(std::move(a)) { std::cout << "B(Array&&)" << std::endl; }
        
        template<typename... Ts>
        requires ((... && std::is_same_v<Ts, A>) && sizeof...(Ts) == N)
        B(Ts&&... content) : arr{ std::forward<Ts>(content)... }   { std::cout << "B(A...)" << std::endl; }


        template<typename... Ints>
        requires ((... && std::is_same_v<Ints, int>) && sizeof...(Ints) == N)
        B(Ints... i)
            : arr { A(i)... } {
            std::cout << "B(i...)" << std::endl;
        }

        // B(B&& b)      : arr(std::move(b.arr))   { std::cout << "B(B&&)"      << std::endl; }
        // B(const B& b) : arr(b.arr)              { std::cout << "B(const B&)" << std::endl; }
        // B& operator=(B&& b)      { arr = std::move(b.arr); std::cout << "B = B&&"      << std::endl; return *this; }
        // B& operator=(const B& b) { arr = b.arr;            std::cout << "B = const B&" << std::endl; return *this; }
        ~B() { std::cout << "~B()" << std::endl; }
};

// class C {
//     C() = delete;
// };

A f() {
    std::cout << "f() begins" << std::endl;
    // A a;                                 // no RVO
    // if (rand() & 1)
    //     a = A(17);
    // else
    //     a = A(18);
    // A a = (rand() & 1) ? A(17) : A(18);  // NRVO
    A a = [] {                              // NRVO
        if (rand() & 1)
            return A(17);
        else
            return A(18);
    } ();
    std::cout << "f() about to return" << std::endl;
    return a;
}

////////////////////////////////////////////////////////

void test_vector() {

    std::vector<A> v;
    v.reserve(3);

    for (int i = 0; i < 3; i++)
        v.emplace_back();
}

void test_malloc() {

    A* t = static_cast<A*>(malloc(3 * sizeof(A)));

    for (int i = 0; i < 3; i++)
        new(&t[i]) A();

    std::cout << "\nDone\n" << std::endl;

    for (int i = 0; i < 3; i++) {
        std::cout << t[i].id << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < 3; i++)
        t[i].~A();

    free(t);
}

void test_allocator() {
    using Alloc = std::allocator<A>;
    using tr = std::allocator_traits<Alloc>;
    Alloc alloc;
    A* pt(tr::allocate(alloc, 3));
    std::span array { pt, 3 };

    for (A& a : array) {
        tr::construct(alloc, &a);
    }

    std::cout << "\nDone\n" << std::endl;

    for (const A& a : array) {
        std::cout << a.id << " ";
    }
    std::cout << std::endl;

    for (A& a : array) {
        tr::destroy(alloc, &a);
    }
}

int main() {

    srand(time(0));
    [[maybe_unused]] A a = f();

    B b1(A(1), A(2), A(3));    // 6 objects created
    B b2(1, 2, 3);             // 3

    custom_stack<A> stack(3);

    for (int i = 0; i < 4; i++) {
        stack.emplace(i);
    }

    return EXIT_SUCCESS;
}