#include "scene/material/normal_map.hpp"

#include "file_readers/normal_map_reader.hpp"

normal_map::normal_map(const unsigned int w, const unsigned int h, vector_matrix&& data)
    : data(std::move(data)),
      width(w - 1), height(h - 1),
      width_real(width),
      height_real(height) {}


/* Constructor from a .bmp file
   Writes true in parsing_successful if the operation was successful */
normal_map::normal_map(const std::string& file_name, bool& parsing_successful) {

    std::optional<vector_matrix> vm_opt = read_normal_map(file_name);
    parsing_successful = vm_opt.has_value();

    data = std::move(vm_opt.value());
    if (parsing_successful) {
        width  = data.size() - 1;
        height = data[0].size() - 1;
        width_real  = static_cast<real>(width);
        height_real = static_cast<real>(height);
    }
}



