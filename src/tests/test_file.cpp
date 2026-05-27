#include "file_readers/file.hpp"

int main() {

    {
        file f("a.txt", "w");
        f.write("12 -3 14  \n");
        f.close();

        const file f2("a.txt");
        int a, b, c;
        f2.scanf("%d%d%d", a, b, c);
        printf("a = %d, b = %d, c = %d\n", a, b, c);
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
            exit(1);
        printf("%.5s c = %.2s\n", t, c.value().t);
    }

    {
        const file f("a.txt", "r");
        const uint32_t a = f.scan<uint32_t>().value();
        const  int32_t b = f.scan< int32_t>().value();
        const uint64_t c = f.scan<uint64_t>().value();
        printf("a = %u, b = %d, c = %llu\n", a, b, c);
    }

    {
        const file f("a.txt", "r");
        const float  x = f.scan<float> ().value();
        const double y = f.scan<double>().value();
        printf("x = %f, y = %lf\n", x, y);
    }

    {
        const file f("a.txt", "a");
        const std::string s = "Alex";
        const unsigned int n = 31;
        f.printf("Je m'appelle %s, j'ai %u ans.\n", s.c_str(), n);
    }

    {
        const file f("a.txt");
        f.cat();
        f.rewind();
        printf("\n");
        f.skip_line();
        f.cat_from();
    }

    return EXIT_SUCCESS;
}