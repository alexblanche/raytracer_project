#include "file_readers/image_files/normal_map_reader.hpp"

#include "file_readers/image_files/bmp_reader.hpp"

static normal_map::vector_matrix convert_bmp_to_normal_field(const matrix& col_data) {

    constexpr real s = 2.0_r / 255.0_r;
    normal_map::vector_matrix norm_data(col_data.height, std::vector<rt::vector>(col_data.width));

    for (int j = 0; const matrix::const_row row : col_data) {
        std::vector<rt::vector>& norm_data_line = norm_data[j];

        for (int i = 0; const rt::color& color : row) {
            // Conversion from [0..255] to [-1;1]
            norm_data_line[i] = rt::vector (
                color.red   * s - 1.0_r,
                color.green * s - 1.0_r,
                color.blue  * s - 1.0_r
            ).unit();
            i++;
        }
        j++;
    }
    
    return norm_data;
}

std::optional<normal_map::vector_matrix> read_normal_map(const std::string& file_name) {

    const std::expected<matrix, file_reader::error> mat_opt = bmp::read_file(file_name);
    if (not mat_opt.has_value())
        return std::nullopt;

    const matrix& col_data = mat_opt.value();
    return convert_bmp_to_normal_field(col_data);
}

