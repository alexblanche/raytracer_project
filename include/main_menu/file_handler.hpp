#pragma once

#include "main_menu/runtime_parameters.hpp"
#include "screen/color.hpp"

#include <vector>


enum class exit_status {
    Success, Failure
};

exit_status operator&&(exit_status s1, exit_status s2) {
    return (s1 == exit_status::Success && s2 == exit_status::Success) ?
          exit_status::Success
        : exit_status::Failure;
}

class file_handler {
    private:
        mutable bool output_dir_exists = false;

        void create_dir() const;

        enum class type {
            Bmp, Raw
        };

        exit_status export_file(const type file_type, const char* filename, const unsigned int number_of_rays,
            const std::vector<std::vector<rt::color>>& matrix, const runtime_parameters& runtime_parameters) const;

    public:
        file_handler();

        inline exit_status export_raw_data(const char* filename, const unsigned int number_of_rays,
            const std::vector<std::vector<rt::color>>& matrix, const runtime_parameters& runtime_parameters) const {
            
            return export_file(type::Raw, filename, number_of_rays, matrix, runtime_parameters);
        }

        inline exit_status export_bmp(const char* filename, const unsigned int number_of_rays,
            const std::vector<std::vector<rt::color>>& matrix, const runtime_parameters& runtime_parameters) const {

            return export_file(type::Bmp, filename, number_of_rays, matrix, runtime_parameters);
        }
};