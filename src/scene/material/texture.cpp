#include "scene/material/texture.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/hdr_reader.hpp"

#include <iostream>
#include <filesystem>

/* Constructors */

// /* Default constructor */
// texture::texture(const int width, const int height, std::vector<std::vector<rt::color>>&& data)
//     : data(std::move(data)),
//         width_minus_one(static_cast<real>(width  - 1)),
//         height_minus_one(static_cast<real>(height - 1)),
//         width(width), height(height) {}

/* Constructor from a .bmp or .hdr file
   Writes true in parsing_successful if the operation was successful */
texture::texture(const std::string& file_name, bool& parsing_successful, const real gamma) {

    const std::string extension = std::filesystem::path(file_name).extension().generic_string();
    const bool is_bmp = extension == ".bmp";
    const bool is_right_format = is_bmp || extension == ".hdr";
    if (not is_right_format) {
        printf("Error in texture definition: wrong file format\n");
        throw;
    }
    
    std::optional<matrix> mat_opt = (is_bmp) ? read_bmp(file_name) : read_hdr(file_name);
    parsing_successful = mat_opt.has_value();
    if (not parsing_successful)
        return;

    data = std::move(mat_opt.value());
    if (gamma != 1.0f)
        data.apply_gamma(gamma);
    width_minus_one  = static_cast<real>(data.width  - 1);
    height_minus_one = static_cast<real>(data.height - 1);
}


/* Returns the color stored in data at UV-coordinates u, v (between 0 and 1) times width, height */
const rt::color& texture::get_color(const real u, const real v) const {
    const int x = u * width_minus_one;
    const int y = v * height_minus_one;
    // Due to floating-point imprecision, some "unit" vector have a norm slightly larger than 1,
    // producing out of range coordinates
    return (x < 0 || x >= data.width || y < 0 || y >= data.height) ?
        data.data[std::min(data.width - 1, std::max(0, x))][std::min(data.height - 1, std::max(0, y))]
        :
        data.data[x][y];
}
