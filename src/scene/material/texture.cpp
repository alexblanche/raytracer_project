#include "scene/material/texture.hpp"

#include "file_readers/image_files/bmp_reader.hpp"
#include "file_readers/image_files/hdr_reader.hpp"

#include <iostream>
#include <filesystem>

/* Constructor from a .bmp or .hdr file
   Writes true in parsing_successful if the operation was successful */
texture::texture(const std::string& file_name, bool& parsing_successful, const real gamma) {

    const std::string extension = std::filesystem::path(file_name).extension().generic_string();
    const bool is_bmp = extension == ".bmp";
    const bool is_right_format = is_bmp || extension == ".hdr";
    if (not is_right_format)
        throw std::runtime_error("Error in texture definition: wrong file format\n");
    
    std::expected<matrix, file_reader::error> mat_opt = (is_bmp) ? bmp::read_file(file_name) : hdr::read_file(file_name);
    parsing_successful = mat_opt.has_value();
    if (not parsing_successful)
        return;

    data = std::move(mat_opt.value());
    if (gamma != 1.0_r)
        data.apply_gamma(gamma);

    width  = data.width  - 1;
    height = data.height - 1;
    width_real  = static_cast<real>(width);
    height_real = static_cast<real>(height);
}

