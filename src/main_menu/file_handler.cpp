#include "main_menu/file_handler.hpp"

#include "file_readers/bmp_reader.hpp"
#include "file_readers/raw_data.hpp"

#include <filesystem>
#include <string>

static constexpr std::string output_dir_name = "../output";
static const std::filesystem::path output_dir(output_dir_name);

void file_handler::create_dir() const {
    if (output_dir_exists)
        return;
    output_dir_exists = std::filesystem::create_directories(output_dir);
}

exit_status file_handler::export_file(
    const type file_type, const char* filename,
    const unsigned int number_of_rays,
    const std::vector<std::vector<rt::color>>& matrix,
    const runtime_parameters& runtime_parameters) const {

    create_dir();
    const std::string path = std::filesystem::path(output_dir).append(filename).generic_string();

    exit_status status;
    switch (file_type) {
        case file_handler::type::Bmp:
            status = write_bmp(
                path.data(),
                matrix,
                number_of_rays,
                runtime_parameters.tone_mapping.gamma_value
            );
            break;
        
        case file_handler::type::Raw:
            status = export_raw(path.data(), number_of_rays, matrix);
            break;
    }

    switch (status) {
        case exit_status::Success:
            printf("Saved as %s\n", output_dir.filename().append(filename).generic_string().data());
            break;
        
        case exit_status::Failure:
            printf("Save failed\n");
            break;
    }
    return status;
}

file_handler::file_handler()
    : output_dir_exists(
           std::filesystem::exists(output_dir)
        && std::filesystem::is_directory(output_dir)
    ) {}
