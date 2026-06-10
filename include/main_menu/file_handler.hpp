#pragma once

#include "main_menu/runtime_parameters.hpp"
#include "screen/color.hpp"
#include "image/image.hpp"
#include "auxiliary/exit_status.hpp"

#include <vector>

class file_handler {
    private:
        mutable bool output_dir_exists = false;

        void create_dir() const;

        enum class type {
            Bmp, Raw
        };
        using enum type;

        template<type type>
        struct type_filename {
            static constexpr std::string extension = (type == Bmp) ? ".bmp" : ".rtdata";
            
            std::string filename;
            constexpr type_filename(const std::string& name)
                : filename((name.ends_with(extension) ? name : name + extension)) {}
        };

        exit_status export_file(const type file_type, const std::string& filename, const image& image) const;

    public:
        using bmp_filename = type_filename<Bmp>;
        using raw_filename = type_filename<Raw>;

        file_handler();

        inline exit_status export_as(const raw_filename& raw, const image& image) const {
            
            return export_file(Raw, raw.filename, image);
        }

        inline exit_status export_as(const bmp_filename& bmp, const image& image) const {

            return export_file(Bmp, bmp.filename, image);
        }
};

constexpr file_handler::bmp_filename bmp(const std::string& filename) {
    return file_handler::bmp_filename(filename);
}
constexpr file_handler::raw_filename raw(const std::string& filename) {
    return file_handler::raw_filename(filename);
}