#include "file_readers/normal_map_reader.hpp"

#include "file_readers/bmp_reader.hpp"

void convert_bmp_to_normal_field(const std::vector<std::vector<rt::color>>& col_data,
    std::vector<std::vector<rt::vector>>& norm_data,
    const unsigned int width, const unsigned int height) {

    constexpr real s = 2.0f / 255.0f;

    for (unsigned int i = 0; i < width; i++) {
        const std::vector<rt::color>& col_data_line = col_data[i];
        std::vector<rt::vector>& norm_data_line = norm_data[i];
        for (unsigned int j = 0; j < height; j++) {
            const rt::color& col = col_data_line[j];
            // Conversion from [0..255] to [-1;1]
            norm_data_line[j] = rt::vector (
                col.get_red()   * s - 1.0f,
                col.get_green() * s - 1.0f,
                col.get_blue()  * s - 1.0f
            ).unit();
        }
    }
}

bool read_normal_map(const char* file_name, std::vector<std::vector<rt::vector>>& data) {

    const std::optional<dimensions> dims = read_bmp_size(file_name);
    if (not dims.has_value()) return false;

    const unsigned int width  = dims.value().width;
    const unsigned int height = dims.value().height;
    std::vector<std::vector<rt::color>> col_data(width, std::vector<rt::color>(height));
    bool read_success = read_bmp(file_name, col_data);

    if (not read_success) return false;

    data = std::vector<std::vector<rt::vector>>(width, std::vector<rt::vector>(height));
    convert_bmp_to_normal_field(col_data, data, width, height);

    return true;
}

