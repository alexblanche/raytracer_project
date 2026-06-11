#pragma once

#include "legacy/objects/legacy_object.hpp"

class source {
    private:
        rt::vector position;
        rt::color color;

    public:
        source();

        source(const rt::vector& position, const rt::color& color);

        rt::color apply_obj(const hit& h, const std::vector<const object*>& obj_set) const;
};