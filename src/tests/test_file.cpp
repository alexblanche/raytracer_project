#include "file_readers/file.hpp"

int main() {

    {
        file f("a.txt", "w");
        f.write("12 -3 14  ");
        f.close();

        file f2("a.txt");
        int a, b, c;
        f2.scanf("%d%d%d", a, b, c);
        printf("a = %d, b = %d, c = %d\n", a, b, c);
    }

    {
        file f("a.txt", "r");
        char t[5] = { 0 };
        f.read(std::span(t));
        f.skip(1);
        struct obj {
            char t[2] = { 0, 0 };
        };
        const std::optional<obj> c = f.scan<obj>();
        if (not c.has_value())
            exit(1);
        printf("%.5s c = %.2s\n", t, c.value().t);
    }

    {
        file f("a.txt", "r");
        const uint32_t a = f.scan<uint32_t>().value();
        const  int32_t b = f.scan< int32_t>().value();
        const uint64_t c = f.scan<uint64_t>().value();
        printf("a = %u, b = %d, c = %llu\n", a, b, c);
    }

    {
        file f("a.txt", "r");
        const float  x = f.scan<float> ().value();
        const double y = f.scan<double>().value();
        printf("x = %f, y = %lf\n", x, y);
    }

    return EXIT_SUCCESS;
}