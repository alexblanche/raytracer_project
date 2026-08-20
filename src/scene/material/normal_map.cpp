#include "scene/material/normal_map.hpp"

#include "file_readers/image_files/normal_map_reader.hpp"

#include <stdexcept>

normal_map::normal_map(const unsigned int w, const unsigned int h, vector_matrix&& data)
    : data(std::move(data)),
      width(w - 1), height(h - 1),
      width_real(width),
      height_real(height) {}


/* Constructor from a .bmp file */
normal_map::normal_map(const std::string& file_name) {

    std::optional<vector_matrix> vm_opt = read_normal_map(file_name);
    if (not vm_opt.has_value())
        throw std::runtime_error("Error in normal map definition: could not read image file\n");

    data = std::move(vm_opt.value());
    width  = data.size()    - 1;
    height = data[0].size() - 1;
    width_real  = static_cast<real>(width);
    height_real = static_cast<real>(height);
}



