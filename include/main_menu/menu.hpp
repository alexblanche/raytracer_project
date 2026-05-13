#pragma once

#include "main_menu/runtime_parameters.hpp"
#include "main_menu/file_handler.hpp"
#include "file_readers/scene_parser.hpp"

#include <optional>

constexpr char const* default_filename = "../scene.txt";

class menu {

    public:
        runtime_parameters runtime_parameters;
        std::string scene_descriptor_name = default_filename;

        exit_status parse_arguments(int argc, char *argv[]);

        inline std::optional<scene> parse_scene_descriptor_file() const {
            return parse_scene_descriptor(scene_descriptor_name.data());
        }

        void update_gamma(float new_gamma);

        exit_status run(const scene& scene) const;
};