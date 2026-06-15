#pragma once

#include "main_menu/runtime_parameters.hpp"
#include "auxiliary/exit_status.hpp"
#include "file_readers/parsers/scene_parser.hpp"

#include <string>
#include <span>
#include <optional>

static constexpr std::string DEFAULT_DESCRIPTOR_FILE_NAME = "../scene.txt";

class menu {

    public:
        runtime_parameters runtime_parameters;
        std::string scene_descriptor_name = DEFAULT_DESCRIPTOR_FILE_NAME;

        exit_status parse_arguments(const std::span<const std::string> args);

        inline std::optional<scene> parse_scene_descriptor_file() const {
            return parse_scene_descriptor(scene_descriptor_name);
        }

        void update_gamma(float new_gamma);

        exit_status run(const scene& scene) const;

    private:
        exit_status parse_aux(const std::span<const std::string> args);
};