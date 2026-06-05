#include "file_readers/normal_map_reader.hpp"

#include "file_readers/bmp_reader.hpp"

static normal_map::vector_matrix convert_bmp_to_normal_field(const matrix& col_data) {

    constexpr real s = 2.0_r / 255.0_r;
    normal_map::vector_matrix norm_data(col_data.height, std::vector<rt::vector>(col_data.width));

    for (int j = 0; j < col_data.height; j++) {
        const matrix::const_row row = col_data[j];
        std::vector<rt::vector>& norm_data_line = norm_data[j];
        for (int i = 0; i < col_data.width; i++) {
            const rt::color& col = row[i];
            // Conversion from [0..255] to [-1;1]
            norm_data_line[i] = rt::vector (
                col.get_red()   * s - 1.0_r,
                col.get_green() * s - 1.0_r,
                col.get_blue()  * s - 1.0_r
            ).unit();
        }
    }
    
    return norm_data;
}

std::optional<normal_map::vector_matrix> read_normal_map(const std::string& file_name) {

    const std::optional<matrix> mat_opt = read_bmp(file_name);
    if (not mat_opt.has_value())
        return std::nullopt;

    const matrix& col_data = mat_opt.value();
    return convert_bmp_to_normal_field(col_data);
}

