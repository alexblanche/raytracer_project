#include "scene/objects/object.hpp"
#include "scene/material/material.hpp"

#include <optional>

/* Constructors */

object::object() {}

object::object(const rt::vector& position, const unsigned int material_index)
    : position(position),
        material_index(material_index) {}

object::object(const rt::vector& position, const unsigned int material_index, const unsigned int texture_info_index)
    : position(position),
        texture_information_index(texture_info_index),
        material_index(material_index) {}