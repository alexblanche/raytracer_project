#pragma once

#include "main_menu/runtime_parameters.hpp"
#include "screen/color.hpp"

#include "auxiliary/exit_status.hpp"

#include <vector>

class file_handler {
    private:
        mutable bool output_dir_exists = false;

        void create_dir() const;

        enum class type {
            Bmp, Raw
        };

        exit_status export_file(const type file_type, const std::string& filename, const unsigned int number_of_rays,
            const std::vector<std::vector<rt::color>>& matrix, const real gamma = 1.0f) const;

    public:
        file_handler();

        inline exit_status export_raw_data(const std::string& filename, const unsigned int number_of_rays,
            const std::vector<std::vector<rt::color>>& matrix) const {
            
            return export_file(type::Raw, filename, number_of_rays, matrix);
        }

        inline exit_status export_bmp(const std::string& filename, const unsigned int number_of_rays,
            const std::vector<std::vector<rt::color>>& matrix, const real gamma) const {

            return export_file(type::Bmp, filename, number_of_rays, matrix, gamma);
        }
};