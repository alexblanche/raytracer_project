#include "main_menu/file_handler.hpp"

#include "file_readers/bmp_reader.hpp"
#include "file_readers/raw_data.hpp"

#include <filesystem>
#include <string>

using namespace std::filesystem;

static constexpr std::string OUTPUT_DIR_NAME = "../output";
static const path output_dir(OUTPUT_DIR_NAME);

file_handler::file_handler()
    : output_dir_exists(exists(output_dir) && is_directory(output_dir)) {}

void file_handler::create_dir() const {
    if (output_dir_exists)
        return;
    output_dir_exists = create_directories(output_dir);
}

exit_status file_handler::export_file(const type file_type, const std::string& filename, const image& image) const {

    create_dir();
    const std::string file_path = path(output_dir).append(filename).generic_string();

    using enum file_handler::type;
    exit_status status;
    switch (file_type) {
        case Bmp:
            status = write_bmp(file_path, image);
            break;
        case Raw:
            status = export_raw(file_path, image);
            break;
    }

    switch (status) {
        case exit_status::Success:
            printf("Saved as %s\n", output_dir.filename().append(filename).generic_string().c_str());
            break;
        
        case exit_status::Failure:
            printf("Save failed\n");
            break;
    }
    return status;
}