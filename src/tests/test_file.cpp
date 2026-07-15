#include "file_readers/file.hpp"

#include <string>
#include <cassert>

[[maybe_unused]]
static void test_basic() {
    constexpr int values[] = { 12, -3, 14 };

    {
        file f("a.txt", "w");
        const std::string s =
                    std::to_string(values[0])
            + " " + std::to_string(values[1])
            + " " + std::to_string(values[2])
            + "  \n";
        f.write(s);
        f.close();

        const file f2("a.txt");
        int a, b, c;
        f2.scanf("%d%d%d", a, b, c);
        printf("a = %d, b = %d, c = %d\n", a, b, c);

        assert(a == values[0] && b == values[1] && c == values[2]);
    }
    
    {
        const std::string s = build_format_string<int, double, float, unsigned long long int, char, char>();
        printf("%s", s.c_str()); printf("\n");
        const std::string s2 = build_format_string_space<int, double, float, unsigned long long int, char, char>();
        printf(s2.c_str(), 3, 2.5, 5.8, 4, 'a', 'b'); printf("\n");
        
        assert(s == "%d%lf%f%llu%c%c");
        assert(s2 == "%d %lf %f %llu %c %c ");
    }

    {
        const file f("a.txt", "r");
        char t[5] = { 0 };
        f.read(std::span(t));
        f.skip(1);
        struct obj {
            char t[2] = { 0, 0 };
        };
        const std::optional<obj> c = f.scan<obj>();
        if (not c.has_value())
            exit(EXIT_FAILURE);
        printf("%.5s c = %.2s\n", t, c.value().t);

        [[maybe_unused]] const auto [ c1, c2 ] = c.value().t;
        assert(c1 == ('0' + ((values[2] / 10) % 10)) && c2 == ('0' + (values[2] % 10)));
    }

    {
        const file f("a.txt", "r");
        const uint32_t a = f.scan<uint32_t>().value();
        const  int32_t b = f.scan< int32_t>().value();
        const uint64_t c = f.scan<uint64_t>().value();
        printf("a = %u, b = %d, c = %llu\n", a, b, c);
        assert(a == values[0] && b == values[1] && c == values[2]);
    }

    {
        const file f("a.txt", "r");
        const float  x = f.scan<float> ().value();
        const double y = f.scan<double>().value();
        printf("x = %f, y = %lf\n", x, y);
        assert(x == 12.0f && y == -3.0);
    }

    {
        const file f("a.txt", "a");
        const std::string s = "Alex";
        const unsigned int n = 31;
        [[maybe_unused]] const exit_status status = f.printf("My name is %s, I am %u.\n", s.c_str(), n);
        assert(status == exit_status::Success);
    }

    {
        const file f("a.txt");
        f.cat();
        f.rewind();
        printf("\n");
        f.skip_line();
        f.cat_from();
    }

    {
        file f("a.txt");
        const auto [ a, b, c ] = f.scan<int, float, double>().value();
        printf("a = %d, b = %f, c = %lf\n", a, b, c);
        assert(a == values[0] && b == values[1] && c == values[2]);

        f.rewind();
        f.cat();
        f.rewind();
        const auto [ x, y, z ] = f.scan<int, 3>();
        printf("Extraction from array: x = %d, y = %d, z = %d\n", x, y, z);
        assert(x == values[0] && y == values[1] && z == values[2]);
    }

    constexpr int intvals[] = { 2, 3, -4 };
    [[maybe_unused]] const auto [ i, j, k ] = intvals;
    {
        file f("a.txt", "w");
        assert(exit_status::Success == f.write<int>(i, j, k));
    }

    {
        file f("a.txt");
        int buffer[3] = { 0 };
        f.read<int, 3>(buffer);
        [[maybe_unused]] const auto [ x, y, z ] = buffer;
        assert(x == i && y == j && z == k);
    }

    printf("\nAll good!\n");
}

int main() {

    test_basic();

    return EXIT_SUCCESS;
}